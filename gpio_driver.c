/**
 * @file    gpio_driver.c
 * @brief   Trinh dieu khien GPIO cap thanh ghi (Bare-metal) cho STM32F103
 * @details Trien khai cau hinh CRL/CRH, doc/ghi nguyen tu qua BSRR/BRR va ngat ngoai EXTI5.
 */

#include "gpio_driver.h"

/**
 * @brief  Khoi tao che do va kieu cau hinh cho mot chan GPIO
 * @details Moi chan GPIO chiem 4 bit cau hinh gom 2 bit MODE va 2 bit CNF:
 *          - Cac chan tu Pin 0 den Pin 7 duoc cau hinh qua thanh ghi CRL.
 *          - Cac chan tu Pin 8 den Pin 15 duoc cau hinh qua thanh ghi CRH.
 */
void gpio_init(GPIO_TypeDef *port, uint8_t pin, gpio_mode_t mode, gpio_cnf_t cnf)
{
    uint32_t pos = pin * 4;             /* Moi chan chiem 4 bit */
    volatile uint32_t *reg;             /* Tro toi thanh ghi CRL hoac CRH */

    if (pin < 8)
    {
        reg = &port->CRL;               /* Cau hinh cho chan 0 .. 7 */
    }
    else
    {
        reg = &port->CRH;               /* Cau hinh cho chan 8 .. 15 */
        pos -= 32;                      /* Dich vi tri bit tuong ung voi CRH */
    }

    /* Xoa 4 bit cau hinh cu (su dung 0xFUL de tranh tran so nguyen co dau o bit 28..31) */
    *reg &= ~(0xFUL << pos);

    /* Ghi 2 bit mode (bit thap) va 2 bit cnf (bit cao) vao vi tri pin */
    *reg |= ((uint32_t)((mode & 0x03) | ((cnf & 0x03) << 2))) << pos;
}

/**
 * @brief  Ghi trang thai logic ra chan GPIO
 * @details Su dung thanh ghi BSRR de Set va BRR de Reset giup thao tac dat trang thai
 *          dien ra nguyen tu (atomic), khong can Read-Modify-Write, an toan tuyet doi trong ngat.
 */
void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t state)
{
    if (state)
    {
        port->BSRR = (1 << pin);        /* Ghi 1 vao BSRR de keo chan len muc 1 (Set) */
    }
    else
    {
        port->BRR = (1 << pin);         /* Ghi 1 vao BRR de keo chan xuong muc 0 (Reset) */
    }
}

/**
 * @brief  Doc trang thai logic tu thanh ghi du lieu vao (IDR)
 */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->IDR & (1 << pin)) ? 1 : 0;
}

/**
 * @brief  Dao trang thai logic chan GPIO qua thanh ghi ODR (dung phep toan XOR)
 */
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    port->ODR ^= (1 << pin);
}

/**
 * @brief  Khoi tao ngat ngoai EXTI5 tren chan PA5 (Chuyen tiep tin hieu OS cua LM75)
 * @details LM75 co ngo ra OS dang Open-Drain Active-LOW (keo xuong 0 khi vuot nguong Tos).
 *          STM32 cau hinh ngat suon xuong (Falling Edge Trigger).
 */
void exti5_init(void)
{
    /* 1. Bat clock cho AFIO (bat buoc de cau hinh dinh tuyen ngat ngoai) */
    RCC->APB2ENR |= (1 << 0);

    /* 2. Dinh tuyen chan PA5 vao duong ngat EXTI5
     *    EXTICR[1] (EXTICR2) quan ly EXTI4 (bit 0..3) va EXTI5 (bit 4..7).
     *    Gia tri 0000 tuong ung voi Port A. */
    AFIO->EXTICR[1] &= ~(0xF << 4);

    /* 3. Cau hinh bo kich hoat ngat ngoai EXTI
     *    - Xoa co ngat rac ban dau: Ghi truc tiep bit 1 vao PR (rc_w1, tuyet doi khong dung |=)
     *    - Mo mat na ngat dong 5 tren IMR
     *    - Kich hoat ngat suon xuong tren FTSR, tat ngat suon len tren RTSR */
    EXTI->PR   = (1 << 5);
    EXTI->IMR  |= (1 << 5);
    EXTI->FTSR |= (1 << 5);
    EXTI->RTSR &= ~(1 << 5);

    /* 4. Cho phep ngat EXTI9_5 tren bo dieu khien ngat NVIC */
    NVIC_EnableIRQ(EXTI9_5_IRQn);
}
