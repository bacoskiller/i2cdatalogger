#include "stm32f10x.h"
#include "delay.h"

/**
 * @brief Khoi tao bo dinh thoi TIM1 phuc vu tao tre phan cung
 *        (Thay the hoan toan vong lap __NOP() lang phi va khong chuan xac)
 *        SystemCoreClock = 72 MHz, APB2 = 72 MHz -> TIM1 Clock = 72 MHz
 */
void delay_init(void)
{
    // Bat clock cho ngoai vi TIM1 tren bus APB2
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    // Prescaler: 72 MHz / (71 + 1) = 1 MHz -> moi tick dem la dung 1 microsecond (1 us)
    TIM1->PSC = (SystemCoreClock / 1000000UL) - 1;
    TIM1->ARR = 0xFFFF;
    TIM1->CR1 = 0;
    TIM1->EGR = TIM_EGR_UG;     // Cap nhat cac thanh ghi shadow
    TIM1->SR  &= ~TIM_SR_UIF;   // Xoa co update
}

/**
 * @brief Delay chinh xac theo microsecond bang bo dem phan cung TIM1
 * @param us So microsecond can delay
 */
void delay_us(uint16_t us)
{
    if (us == 0) return;

    if (!(RCC->APB2ENR & RCC_APB2ENR_TIM1EN)) {
        delay_init();
    }

    TIM1->CNT = 0;
    TIM1->ARR = us;
    TIM1->EGR = TIM_EGR_UG;
    TIM1->SR &= ~TIM_SR_UIF;
    TIM1->CR1 |= TIM_CR1_CEN;

    while (!(TIM1->SR & TIM_SR_UIF));

    TIM1->CR1 &= ~TIM_CR1_CEN;
    TIM1->SR &= ~TIM_SR_UIF;
}

/**
 * @brief Delay chinh xac theo millisecond bang bo dem phan cung TIM1
 * @param ms So millisecond can delay
 */
void delay(uint16_t ms)
{
    if (ms == 0) return;

    if (!(RCC->APB2ENR & RCC_APB2ENR_TIM1EN)) {
        delay_init();
    }

    while (ms--) {
        TIM1->CNT = 0;
        TIM1->ARR = 1000 - 1;       // 1000 us = 1 ms
        TIM1->EGR = TIM_EGR_UG;
        TIM1->SR &= ~TIM_SR_UIF;
        TIM1->CR1 |= TIM_CR1_CEN;

        while (!(TIM1->SR & TIM_SR_UIF));

        TIM1->CR1 &= ~TIM_CR1_CEN;
        TIM1->SR &= ~TIM_SR_UIF;
    }
}
