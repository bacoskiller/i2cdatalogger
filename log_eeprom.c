/**
 * @file    log_eeprom.c
 * @brief   Module quan ly nhat ky du lieu tren EEPROM khong dung Header (Zero-Header Wear Leveling)
 * @details Trien khai bo dem vong 50 phan tu, moi phan tu 8 byte.
 *          Dung cap Magic Bytes (0xAA 0x55) o cuoi moi ban ghi ket hop so sanh Timestamp
 *          de tim ra ban ghi moi nhat sau moi lan Reset/Khoi dong lai.
 */

#include "stm32f10x.h"
#include "eeprom_i2c.h"
#include "log_eeprom.h"
#include "delay.h"

/* Cau hinh phan vung luu tru */
#define LOG_BASE       0x0000u   /**< Dia chi bat dau luu tru tren EEPROM */
#define LOG_MAX_SLOTS  50u       /**< So luong ban ghi toi da trong bo dem vong */
#define LOG_REC_SIZE   8u        /**< Kich thuoc moi ban ghi (8 bytes * 8 = 64 bytes = 1 Page AT24C256C) */

static uint16_t log_cnt = 0;     /**< Tong so ban ghi hop le hien co */
static uint16_t head_idx = 0;    /**< Chi so slot se ghi tiep theo */

/**
 * @brief  Khoi dong he thong luu tru - Thuat toan tu khoi phuc con tro thong minh
 * @details Quet 50 slot tren EEPROM, kiem tra Magic Bytes (0xAA 0x55) va tim Timestamp cuc dai.
 *          Giai phap loai bo hoan toan vung Header co dinh, tranh tap trung ghi de len Trang 0
 *          giup phan bo do mon dong deu tren toan bo vung nho (True Wear-Leveling).
 */
void log_init(void)
{
    uint8_t buf[LOG_REC_SIZE];
    uint32_t max_ts = 0;
    int16_t newest_slot = -1;
    uint16_t valid_cnt = 0;

    log_cnt = 0;
    head_idx = 0;

    /* Quet toan bo cac slot tren EEPROM */
    for (uint16_t i = 0; i < LOG_MAX_SLOTS; i++)
    {
        uint16_t addr = LOG_BASE + (i * LOG_REC_SIZE);
        
        /* Doc 8 byte cua slot */
        if (eeprom_read_buffer(addr, buf, LOG_REC_SIZE) == I2C_OK)
        {
            /* Kiem tra cap byte nhan dien hop le (Magic Bytes) */
            if (buf[6] == 0xAA && buf[7] == 0x55)
            {
                valid_cnt++;
                
                /* Giai ma Timestamp 32-bit (Big-Endian) */
                uint32_t ts = ((uint32_t)buf[0] << 24) |
                              ((uint32_t)buf[1] << 16) |
                              ((uint32_t)buf[2] << 8)  |
                              ((uint32_t)buf[3]);

                /* Xac dinh slot co Timestamp lon nhat (mau moi nhat truoc khi mat nguon) */
                if (newest_slot == -1 || ts > max_ts)
                {
                    max_ts = ts;
                    newest_slot = i;
                }
            }
        }
        delay(1);
    }

    log_cnt = valid_cnt;

    /* Neu tim thay it nhat mot ban ghi hop le */
    if (newest_slot != -1)
    {
        /* Vi tri ghi tiep theo la slot lien sau slot moi nhat (xoay vong) */
        head_idx = (newest_slot + 1) % LOG_MAX_SLOTS;
    }
    else
    {
        /* Bo nho EEPROM con trang hoac da bi xoa toan bo */
        head_idx = 0;
        log_cnt = 0;
    }
}

/**
 * @brief  Ghi mot ban ghi moi vao EEPROM
 * @details Chi ghi dung 1 lan 8 byte (Page Write), khong can phai ghi them vao Header.
 *          Giam 50% thoi gian chiem dung bus I2C va han che toi da tan suat hao mon chip.
 */
i2c_status_t log_append(uint32_t timestamp, int16_t temp_x10)
{
    uint8_t buf[LOG_REC_SIZE];
    
    /* 1. Dong goi du lieu 8 byte theo chuan Big-Endian */
    buf[0] = (uint8_t)(timestamp >> 24);
    buf[1] = (uint8_t)(timestamp >> 16);
    buf[2] = (uint8_t)(timestamp >> 8);
    buf[3] = (uint8_t)(timestamp);
    buf[4] = (uint8_t)((uint16_t)temp_x10 >> 8);
    buf[5] = (uint8_t)((uint16_t)temp_x10 & 0xFF);
    buf[6] = 0xAA; /* Magic Byte 1 */
    buf[7] = 0x55; /* Magic Byte 2 */

    /* 2. Tinh dia chi tuyet doi cua slot hien tai */
    uint16_t addr = LOG_BASE + (head_idx * LOG_REC_SIZE);

    /* 3. Ghi khoi 8 byte xuong EEPROM */
    i2c_status_t st = eeprom_write_buffer(addr, buf, LOG_REC_SIZE);
    if (st != I2C_OK) return st;

    /* 4. Cho hoan tat chu trinh ghi vat ly (tWR <= 5ms tren AT24C256C) */
    delay(10);

    /* 5. Tinh tien con tro ghi va cap nhat so luong mau tren RAM */
    head_idx = (head_idx + 1) % LOG_MAX_SLOTS;
    if (log_cnt < LOG_MAX_SLOTS)
    {
        log_cnt++;
    }

    return I2C_OK;
}

/**
 * @brief  Doc mot ban ghi theo thu tu thoi gian giam dan (back_index = 0 la moi nhat)
 */
uint8_t log_get(uint16_t back_index, LogRecord_t *record)
{
    if (log_cnt == 0 || back_index >= log_cnt || record == 0)
    {
        return 0; /* Tham so khong hop le hoac chua co ban ghi */
    }

    /* Cong thuc truy xuat nguoc trong bo dem vong */
    int16_t target_idx = (int16_t)head_idx - 1 - (int16_t)back_index;
    if (target_idx < 0)
    {
        target_idx += LOG_MAX_SLOTS;
    }

    uint16_t addr = LOG_BASE + ((uint16_t)target_idx * LOG_REC_SIZE);
    uint8_t buf[LOG_REC_SIZE];

    /* Doc 8 byte cua slot */
    if (eeprom_read_buffer(addr, buf, LOG_REC_SIZE) != I2C_OK)
    {
        return 0;
    }

    /* Kiem tra tinh toan ven qua Magic Bytes */
    if (buf[6] != 0xAA || buf[7] != 0x55)
    {
        return 0;
    }

    /* Giai ma du lieu ra cau truc LogRecord_t */
    record->timestamp = ((uint32_t)buf[0] << 24) |
                        ((uint32_t)buf[1] << 16) |
                        ((uint32_t)buf[2] << 8)  |
                        ((uint32_t)buf[3]);
    record->temperature = (int16_t)(((uint16_t)buf[4] << 8) | buf[5]);
    record->reserved = 0;

    return 1;
}

/**
 * @brief  Xoa toan bo nhat ky (Ghi de 0xFF vao Magic Bytes de huy hieu luc cac slot)
 */
i2c_status_t log_clear(void)
{
    uint8_t invalid_magic[2] = {0xFF, 0xFF};

    for (uint16_t i = 0; i < LOG_MAX_SLOTS; i++)
    {
        /* Ghi vao 2 byte cuoi cua slot (vi tri luu Magic Bytes) */
        uint16_t addr = LOG_BASE + (i * LOG_REC_SIZE) + 6;
        eeprom_write_buffer(addr, invalid_magic, 2);
        delay(6);
    }

    log_cnt = 0;
    head_idx = 0;
    return I2C_OK;
}

/**
 * @brief  Lay tong so ban ghi hop le hien co
 */
uint16_t log_count(void)
{
    return log_cnt;
}

/**
 * @brief  Lay vi tri slot tiep theo se duoc ghi
 */
uint16_t log_head(void)
{
    return head_idx;
}
