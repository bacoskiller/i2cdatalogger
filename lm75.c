/**
 * @file    lm75.c
 * @brief   Trinh dieu khien cam bien nhiet do so I2C LM75
 * @details Giai ma so bu 2 Fixed-Point (don vi 0.1 C), loc buoc nhay nhieu Glitch
 *          va kiem tra toan ven bit khong dung D4..D0 theo Datasheet TI/NXP.
 */

#include "stm32f10x.h"
#include "lm75.h"
#include "delay.h"
#include "i2c_driver.h"

#define LM75_REG_TEMP          0x00  // Thanh ghi du lieu nhiet do 16-bit (Read-only)
#define LM75_REG_THYST         0x02  // Thanh ghi nguong nha canh bao (R/W)
#define LM75_REG_TOS           0x03  // Thanh ghi nguong ngat canh bao (R/W)

#define LM75_RETRY             3     // So lan thu lai toi da khi I2C ban
#define LM75_MIN_X10         (-550)  // Gioi han do nho nhat: -55.0 C (Datasheet)
#define LM75_MAX_X10          1250   // Gioi han do lon nhat: +125.0 C (Datasheet)
#define LM75_MAX_JUMP_X10      100   // Nguong phat hien buoc nhay: > 10.0 C / chu ky
#define LM75_CAND_TOL_X10        5   // Dung sai ung vien: <= 0.5 C coi la cung muc
#define LM75_JUMP_CONFIRM        3   // Can 3 mau lien tiep de xac nhan buoc nhay thuc

/* Che do kiem tra phan cung that */
#define LM75_CHECK_UNUSED_BITS   1   // Kiem tra 5 bit D4..D0 phai bang 0 de bat loi ho bus 0xFFFF
#define LM75_VERIFY_READBACK     1   // Doc lai de xac minh thanh ghi sau khi ghi tren mach that

static int16_t last_good = 0;        // Mau nhiet do hop le gan nhat
static uint8_t have_last = 0;
static int16_t cand      = 0;        // Gia tri ung vien khi co buoc nhay dot ngot
static uint8_t cand_cnt  = 0;

static int32_t absd(int32_t a, int32_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

/**
 * @brief Chuyen doi ma trang thai I2C thanh ma trang thai cam bien LM75
 */
static lm75_status_t map_status(i2c_status_t s)
{
    switch (s)
    {
        case I2C_OK:      return LM75_OK;
        case I2C_ERR_BUS: return LM75_ERR_BUS;
        default:          return LM75_ERR_NO_RESP;   // NACK hoac TIMEOUT
    }
}

/**
 * @brief Doc gia tri 16-bit tu thanh ghi LM75 qua bus I2C
 */
static lm75_status_t lm75_read_reg16(uint8_t reg, uint16_t *val)
{
    uint8_t buf[2];
    i2c_status_t st = I2C_ERR_TIMEOUT;

    for (uint8_t n = 0; n < LM75_RETRY; n++)
    {
        st = i2c_write_byte(LM75_I2C_ADDR, reg);
        if (st == I2C_OK) st = i2c_read_mul_bytes(LM75_I2C_ADDR, buf, 2);
        if (st == I2C_OK)
        {
            *val = (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
            return LM75_OK;
        }
        delay(2);
    }
    return map_status(st);
}

/**
 * @brief Ghi gia tri 16-bit vao thanh ghi LM75 qua bus I2C
 */
static lm75_status_t lm75_write_reg16(uint8_t reg, uint16_t val)
{
    uint8_t buf[3];
    i2c_status_t st = I2C_ERR_TIMEOUT;

    buf[0] = reg;
    buf[1] = (uint8_t)(val >> 8);
    buf[2] = (uint8_t)(val & 0xFF);

    for (uint8_t n = 0; n < LM75_RETRY; n++)
    {
        st = i2c_write_mul_bytes(LM75_I2C_ADDR, buf, 3);
        if (st == I2C_OK) return LM75_OK;
        delay(2);
    }
    return map_status(st);
}

/**
 * @brief Khoi tao nguong Tos va Thyst cho cam bien LM75
 */
lm75_status_t LM75_Init(uint8_t tos_c, uint8_t thyst_c)
{
    lm75_status_t st;
    uint16_t tos_val   = (uint16_t)((uint16_t)tos_c   << 8);   // Vi du: 40 C -> 0x2800
    uint16_t thyst_val = (uint16_t)((uint16_t)thyst_c << 8);   // Vi du: 39 C -> 0x2700

    st = lm75_write_reg16(LM75_REG_TOS, tos_val);     if (st != LM75_OK) return st;
    st = lm75_write_reg16(LM75_REG_THYST, thyst_val); if (st != LM75_OK) return st;

#if LM75_VERIFY_READBACK
    {
        uint16_t rb;
        // Chi so sanh 9 bit cao co nghia (D15..D7)
        st = lm75_read_reg16(LM75_REG_TOS, &rb);
        if (st != LM75_OK) return st;
        if ((rb & 0xFF80u) != (tos_val & 0xFF80u)) return LM75_ERR_BAD_VALUE;

        st = lm75_read_reg16(LM75_REG_THYST, &rb);
        if (st != LM75_OK) return st;
        if ((rb & 0xFF80u) != (thyst_val & 0xFF80u)) return LM75_ERR_BAD_VALUE;
    }
#endif
    return LM75_OK;
}

/**
 * @brief Dat lai trang thai bo loc buoc nhay
 */
void LM75_ResetFilter(void)
{
    have_last = 0;
    cand_cnt  = 0;
}

/**
 * @brief Doc va giai ma nhiet do thoi gian thuc (don vi 0.1 C)
 */
lm75_status_t LM75_ReadTemp(int16_t *temp_x10)
{
    uint16_t raw = 0;
    int32_t t;
    lm75_status_t st = lm75_read_reg16(LM75_REG_TEMP, &raw);
    if (st != LM75_OK) return st;

#if LM75_CHECK_UNUSED_BITS
    // Neu bus bi tha noi (0xFFFF) hoac tuot day, 5 bit thap se khac 0
    if (raw & 0x001F) return LM75_ERR_BAD_VALUE;
#endif

    // Giai ma so bu 2 Fixed-Point: Dich phai 5 bit so hoc (ASRS) giu nguyen bit dau am
    // (raw >> 5) * 0.5 C = ((raw >> 5) * 125) / 100 de chuyen sang don vi 0.1 C
    t = (int32_t)((int16_t)raw >> 5) * 125 / 100;

    // Kiem tra gioi han vat ly cam bien (-55.0 C den +125.0 C)
    if (t < LM75_MIN_X10 || t > LM75_MAX_X10) return LM75_ERR_BAD_VALUE;

    // Bo loc buoc nhay (Glitch Rejection Filter):
    // Neu mau nhay vot > 10.0 C so voi lan doc truoc, phai lap lai on dinh 3 lan moi chap nhan
    if (have_last && absd(t, last_good) > LM75_MAX_JUMP_X10)
    {
        if (cand_cnt > 0 && absd(t, cand) <= LM75_CAND_TOL_X10) cand_cnt++;
        else { cand = (int16_t)t; cand_cnt = 1; }

        if (cand_cnt < LM75_JUMP_CONFIRM)
        {
            *temp_x10 = last_good; // Giu mau hop le truoc do de giao dien LCD khong chop giat
            return LM75_OK;
        }
    }

    last_good = (int16_t)t;
    have_last = 1;
    cand_cnt  = 0;
    *temp_x10 = (int16_t)t;
    return LM75_OK;
}
