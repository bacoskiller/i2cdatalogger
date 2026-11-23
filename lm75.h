/**
 * @file    lm75.h
 * @brief   Giao tiep cam bien nhiet do so I2C LM75 / LM75A (TI, NXP)
 * @details Doc nhiet do do phan giai 9-bit, cau hinh nguong canh bao qua nhiet Tos va Thyst.
 */

#ifndef LM75_H
#define LM75_H

#include <stdint.h>

#define LM75_I2C_ADDR  0x48          // Dia chi I2C 7-bit (A2 A1 A0 = 000)

typedef enum {
    LM75_OK = 0,                     // Doc/ghi thanh cong
    LM75_ERR_NO_RESP,                // Khong phan hoi (NACK hoac Timeout)
    LM75_ERR_BUS,                    // Loi phan cung bus I2C (BERR, ARLO)
    LM75_ERR_BAD_VALUE               // Gia tri sai bat thuong / ho bus
} lm75_status_t;

/**
 * @brief  Khoi tao cam bien LM75, cai dat nguong Tos va Thyst
 * @param  tos_c:   Nhiet do ngat canh bao (do C, vi du 40)
 * @param  thyst_c: Nhiet do nha canh bao (do C, vi du 39)
 */
lm75_status_t LM75_Init(uint8_t tos_c, uint8_t thyst_c);

/**
 * @brief  Doc nhiet do thoi gian thuc tu cam bien LM75 (don vi 0.1 C, co dau)
 * @param  temp_x10: Con tro nhan gia tri nhiet do (vi du 255 -> 25.5 C)
 */
lm75_status_t LM75_ReadTemp(int16_t *temp_x10);

/**
 * @brief  Dat lai trang thai bo loc buoc nhay nhieu (Glitch Filter)
 */
void LM75_ResetFilter(void);

#endif /* LM75_H */
