/**
 * @file    lcd_i2c.c
 * @brief   Trinh dieu khien man hinh LCD 16x2 qua chip mo rong I2C PCF8574
 * @details Anh xa bit cua PCF8574 toi HD44780:
 *          P0: RS (0 = Lenh, 1 = Du lieu)
 *          P1: RW (0 = Ghi, 1 = Doc)
 *          P2: EN (1 = Bat dau chot xung, 0 = Chot du lieu)
 *          P3: Backlight (0x08 = Bat den nen LED)
 *          P4..P7: Du lieu 4-bit (D4 .. D7 cua LCD)
 */

#include "stm32f10x.h"
#include "i2c_driver.h"
#include "lcd_i2c.h"
#include "delay.h"

/**
 * @brief  Gui mot byte lenh dieu khien toi LCD (chia thanh 2 nibble 4-bit)
 * @param  cmd: Ma lenh dieu khien
 */
void lcd_send_cmd(char cmd)
{
    char data_u, data_l;
    uint8_t data_lcd[4];

    data_u = (char)(cmd & 0xF0);         /* 4 bit cao */
    data_l = (char)((cmd << 4) & 0xF0);  /* 4 bit thap */

    /* Gui 4 byte qua I2C (2 byte cho nibble cao, 2 byte cho nibble thap)
     * Bitmask 0x0C = EN=1, Backlight=1, RS=0, RW=0
     * Bitmask 0x08 = EN=0, Backlight=1, RS=0, RW=0 (tao suon xuong de chot lenh) */
    data_lcd[0] = (uint8_t)(data_u | 0x0C);
    data_lcd[1] = (uint8_t)(data_u | 0x08);
    data_lcd[2] = (uint8_t)(data_l | 0x0C);
    data_lcd[3] = (uint8_t)(data_l | 0x08);

    i2c_write_mul_bytes(LCD_I2C_ADDR, data_lcd, 4);
}

/**
 * @brief  Gui mot byte ky tu hien thi toi LCD (RS = 1)
 * @param  data: Ky tu ASCII can hien thi
 */
void lcd_send_data(char data)
{
    char data_u, data_l;
    uint8_t data_t[4];

    data_u = (char)(data & 0xF0);        /* 4 bit cao */
    data_l = (char)((data << 4) & 0xF0); /* 4 bit thap */

    /* Bitmask 0x0D = EN=1, Backlight=1, RS=1, RW=0
     * Bitmask 0x09 = EN=0, Backlight=1, RS=1, RW=0 (tao suon xuong de chot du lieu) */
    data_t[0] = (uint8_t)(data_u | 0x0D);
    data_t[1] = (uint8_t)(data_u | 0x09);
    data_t[2] = (uint8_t)(data_l | 0x0D);
    data_t[3] = (uint8_t)(data_l | 0x09);

    i2c_write_mul_bytes(LCD_I2C_ADDR, data_t, 4);
}

/**
 * @brief  Khoi tao LCD theo trinh tu 4-bit cua HD44780
 */
void lcd_init(void)
{
    delay(50);              /* Cho nguon dien on dinh (>40ms sau VCC dat 4.5V) */

    /* Trinh tu khoi dong buoc vao che do 4-bit */
    lcd_send_cmd(0x03);     /* Khoi tao lan 1 */
    delay(5);
    lcd_send_cmd(0x03);     /* Khoi tao lan 2 */
    delay(1);
    lcd_send_cmd(0x03);     /* Khoi tao lan 3 */
    delay(10);
    lcd_send_cmd(0x02);     /* Thiet lap sang che do 4-bit (Function Set 4-bit) */
    delay(10);

    /* Cau hinh chuc nang hoat dong */
    lcd_send_cmd(0x28);     /* 4-bit mode, 2 dong hien thi, ma tran font 5x8 */
    delay(1);
    lcd_send_cmd(0x08);     /* Tat hien thi tam thoi (Display OFF) */
    delay(1);
    lcd_send_cmd(0x01);     /* Xoa toan bo man hinh (Clear Display) */
    delay(2);
    lcd_send_cmd(0x06);     /* Entry mode: Tu dong tang con tro sang phai, khong dich man hinh */
    delay(1);
    lcd_send_cmd(0x0C);     /* Bat man hinh (Display ON), tat con tro (Cursor OFF) */
}

/**
 * @brief  Thiet lap vi tri con tro hien thi tren man hinh
 * @param  a: Cot tu 0 den 15
 * @param  b: Dong (0 la dong 1 - dia chi 0x80; 1 la dong 2 - dia chi 0xC0)
 */
void set_cursor(int a, int b)
{
    switch (b)
    {
        case 0:
            lcd_send_cmd((char)(0x80 + a));
            break;
        case 1:
            lcd_send_cmd((char)(0xC0 + a));
            break;
        default:
            break;
    }
}

/**
 * @brief  Gui chuoi ky tu ra man hinh
 * @param  str: Chuoi ASCII ket thuc bang ky tu '\0'
 */
void lcd_send_string(char *str)
{
    while (*str)
    {
        lcd_send_data(*str++);
    }
}

/**
 * @brief  Xoa man hinh va cho toi thieu 2ms theo thoi gian dap ung cua HD44780
 */
void lcd_clear(void)
{
    lcd_send_cmd(0x01);     /* Lenh Clear Display */
    delay(2);
}

/**
 * @brief  Bat hien thi man hinh, an con tro nhap nhay
 */
void lcd_display_on(void)
{
    lcd_send_cmd(0x0C);     /* Display ON, Cursor OFF */
}

/**
 * @brief  Tat toan bo hien thi man hinh
 */
void lcd_display_off(void)
{
    lcd_send_cmd(0x08);     /* Display OFF */
}
