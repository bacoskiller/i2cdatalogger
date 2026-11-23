/**
 * @file    dem.h
 * @brief   Khai bao may trang thai FSM, SysTick Timer va cac ngat ngoai EXTI
 */

#ifndef DEM_H_
#define DEM_H_

#include <stdint.h>

/**
 * @brief  May trang thai chinh cua Data Logger (goi dinh ky trong while(1))
 *         - STATE_OFF:      He thong nghi, cho nut START (PA0)
 *         - STATE_RUNNING:  Do nhiet do dinh ky, ghi EEPROM moi 5s, hien thi LCD
 *         - STATE_VIEW_LOG: Xem lai 50 ban ghi trong EEPROM qua nut PA2
 *         - STATE_ALARM:    Canh bao qua nhiet khi T >= 40.0 C hoac chan OS keo xuong 0
 */
void dem_sp(void);

/**
 * @brief  Khoi tao bo dinh thoi thoi gian thuc SysTick Timer
 *         Tao ngat chu ky 100ms, cap nhat bien system_time moi 1.0 giay.
 */
void systick_init(void);

/**
 * @brief  Chuyen doi nhiet do gia tri x10 (vd 255 -> 25.5 C) thanh chuoi ky tu
 * @param  value: Gia tri nhiet do (don vi 0.1 C, co dau)
 * @param  buf:   Bo dem chua chuoi ky tu ket qua (toi thieu 7 byte: " 25.5\0")
 */
void format_temp_signed(int16_t value, char *buf);

/* --- Cac ham khoi tao va trinh phuc vu ngat ngoai EXTI --- */
void exti0_init(void);          // PA0 - Nut START (Khoi dong)
void EXTI0_IRQHandler(void);

void exti1_init(void);          // PA1 - Nut STOP (Dung he thong)
void EXTI1_IRQHandler(void);

void exti2_init(void);          // PA2 - Nut READ (Xem lich su log)
void EXTI2_IRQHandler(void);

void exti3_init(void);          // PA3 - Nut CLEAR (Xoa du lieu EEPROM)
void EXTI3_IRQHandler(void);

// EXTI5 (PA5 - Chan OS cua LM75): exti5_init() duoc dinh nghia tai gpio_driver.c
// Trinh phuc vu ngat EXTI9_5_IRQHandler nam trong dem.c

#endif /* DEM_H_ */
