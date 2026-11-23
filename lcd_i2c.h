/**
 * @file    lcd_i2c.h
 * @brief   Trinh dieu khien man hinh LCD 16x2 qua chip mo rong I2C PCF8574
 * @details Ho tro giao tiep 4-bit mode, dieu khien den nen, con tro va hien thi chuoi ky tu.
 */

#ifndef LCD_I2C_H_
#define LCD_I2C_H_

#include <stdint.h>

/* Dia chi I2C 7-bit cua module mo rong PCF8574 (mac dinh 0x27) */
#define LCD_I2C_ADDR    0x27

/**
 * @brief  Khoi tao man hinh LCD theo trinh tu chuan 4-bit mode cua HD44780
 */
void lcd_init(void);

/**
 * @brief  Gui ma lenh dieu khien toi LCD
 * @param  cmd: Ma lenh (Command Code)
 */
void lcd_send_cmd(char cmd);

/**
 * @brief  Gui du lieu ky tu hien thi toi LCD
 * @param  data: Ma ASCII ky tu can hien thi
 */
void lcd_send_data(char data);

/**
 * @brief  Thiet lap vi tri con tro tren man hinh
 * @param  a: Vi tri cot (0 .. 15)
 * @param  b: Vi tri dong (0 = Dong 1, 1 = Dong 2)
 */
void set_cursor(int a, int b);

/**
 * @brief  Gui mot chuoi ky tu ASCII ra man hinh
 * @param  str: Con tro toi chuoi ky tu (ket thuc bang '\0')
 */
void lcd_send_string(char *str);

/**
 * @brief  Xoa toan bo noi dung man hinh va dua con tro ve vi tri (0, 0)
 */
void lcd_clear(void);

/**
 * @brief  Bat hien thi man hinh (Display ON, Cursor OFF)
 */
void lcd_display_on(void);

/**
 * @brief  Tat hien thi man hinh (Display OFF)
 */
void lcd_display_off(void);

#endif /* LCD_I2C_H_ */
