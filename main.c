/**
 * @file    main.c
 * @brief   Diem khoi dau chuong trinh Datalogger nhiet do qua bus I2C (STM32F103)
 * @details Khoi tao ngoai vi bare-metal (TIM1, I2C1, LCD, GPIO, EXTI, SysTick)
 *          va dieu phoi chu trinh may trang thai chinh.
 */

#include "stm32f10x.h"
#include "gpio_driver.h"
#include "lcd_i2c.h"
#include "i2c_driver.h"
#include "delay.h"
#include "dem.h"
#include "eeprom_i2c.h"

int main(void)
{
    // 1. Khoi tao he thong xung nhip (Clock 72 MHz)
    SystemInit();

    // 2. Khoi tao bo dinh thoi TIM1 cho delay phan cung microsecond/millisecond
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    delay_init();

    // 3. Khoi tao bus I2C1 (100 kHz, chuan STM32 RM0008, chia se cho LM75, EEPROM va LCD)
    i2c_init();

    // 4. Khoi tao man hinh LCD 16x2 qua chip I2C PCF8574
    lcd_init();

    // 5. Bat clock ngoai vi GPIO Port A va Port B
    gpio_enable_clock(GPIOA);
    //gpio_enable_clock(GPIOB);

    // 6. Cau hinh cac chan nut nhan va chan bao dong OS (Input Pull-up, ODR = 1)
    gpio_init(GPIOA, 0, GPIO_MODE_INPUT, GPIO_CNF_PU_PD);   // PA0: Nut START (Bat he thong)
    GPIOA->ODR |= (1 << 0);

    gpio_init(GPIOA, 1, GPIO_MODE_INPUT, GPIO_CNF_PU_PD);   // PA1: Nut STOP (Tat he thong)
    GPIOA->ODR |= (1 << 1);

    gpio_init(GPIOA, 2, GPIO_MODE_INPUT, GPIO_CNF_PU_PD);   // PA2: Nut READ (Xem lich su log)
    GPIOA->ODR |= (1 << 2);

    gpio_init(GPIOA, 3, GPIO_MODE_INPUT, GPIO_CNF_PU_PD);   // PA3: Nut XOA EEPROM
    GPIOA->ODR |= (1 << 3);

    gpio_init(GPIOA, 5, GPIO_MODE_INPUT, GPIO_CNF_PU_PD);   // PA5: Chan bao dong OS tu LM75 (Active-LOW)
    GPIOA->ODR |= (1 << 5);

    // 7. Khoi tao cac ngat ngoai EXTI tuong ung (Suon xuong - Falling Edge)
    exti0_init();   // EXTI0 cho PA0
    exti1_init();   // EXTI1 cho PA1
    exti2_init();   // EXTI2 cho PA2
    exti3_init();   // EXTI3 cho PA3
    exti5_init();   // EXTI5 cho PA5 (Chan OS LM75)

    // 8. Khoi tao bo dinh thoi thoi gian thuc SysTick Timer (1 tick moi 1 giay)
    systick_init();

    // 9. Cho phep ngat toan cuc
    __enable_irq();

    // 10. Vong lap vo tan dieu phoi FSM
    while (1) {
        dem_sp();
    }
}
