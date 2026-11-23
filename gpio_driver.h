/**
 * @file    gpio_driver.h
 * @brief   Trinh dieu khien GPIO cap thanh ghi (Bare-metal) cho STM32F103
 * @details Ho tro cau hinh CRL/CRH, doc/ghi nguyen tu qua BSRR/BRR va ngat ngoai EXTI5.
 */

#ifndef GPIO_DRIVER_H_
#define GPIO_DRIVER_H_

#include "stm32f10x.h"

/**
 * @brief  Che do hoat dong cua chan GPIO (MODE[1:0])
 */
typedef enum {
    GPIO_MODE_INPUT      = 0x0,  /**< Che do Input (tan so bang 0) */
    GPIO_MODE_OUTPUT_10M = 0x1,  /**< Che do Output toc do toi da 10 MHz */
    GPIO_MODE_OUTPUT_2M  = 0x2,  /**< Che do Output toc do toi da 2 MHz */
    GPIO_MODE_OUTPUT_50M = 0x3   /**< Che do Output toc do toi da 50 MHz */
} gpio_mode_t;

/**
 * @brief  Kieu cau hinh chuc nang cho chan GPIO (CNF[1:0])
 */
typedef enum {
    /* Khi o che do Input (MODE = 00) */
    GPIO_CNF_ANALOG      = 0x0,  /**< Input che do Analog */
    GPIO_CNF_FLOATING    = 0x1,  /**< Input noi (Floating Input) */
    GPIO_CNF_PU_PD       = 0x2,  /**< Input co dien tro keo len/keo xuong (Pull-up / Pull-down) */

    /* Khi o che do Output (MODE > 00) */
    GPIO_CNF_GP_PP       = 0x0,  /**< General Purpose Output Push-Pull */
    GPIO_CNF_GP_OD       = 0x1,  /**< General Purpose Output Open-Drain */
    GPIO_CNF_AF_PP       = 0x2,  /**< Alternate Function Output Push-Pull */
    GPIO_CNF_AF_OD       = 0x3   /**< Alternate Function Output Open-Drain */
} gpio_cnf_t;

/**
 * @brief  Cap clock APB2 cho Port GPIO tuong ung
 * @param  port: Con tro toi ngoai vi GPIO (GPIOA, GPIOB, GPIOC, ...)
 */
static inline void gpio_enable_clock(GPIO_TypeDef *port)
{
    if      (port == GPIOA) RCC->APB2ENR |= (1 << 2);
    else if (port == GPIOB) RCC->APB2ENR |= (1 << 3);
    else if (port == GPIOC) RCC->APB2ENR |= (1 << 4);
    else if (port == GPIOD) RCC->APB2ENR |= (1 << 5);
    else if (port == GPIOE) RCC->APB2ENR |= (1 << 6);
}

/**
 * @brief  Khoi tao che do va kieu cau hinh cho mot chan GPIO
 * @param  port: Port GPIO (GPIOA, GPIOB, GPIOC, ...)
 * @param  pin:  Chi so chan (0 .. 15)
 * @param  mode: Che do (Input hoac Output voi toc do mong muon)
 * @param  cnf:  Kieu cau hinh (Push-pull, Open-drain, Pull-up, ...)
 */
void gpio_init(GPIO_TypeDef *port, uint8_t pin, gpio_mode_t mode, gpio_cnf_t cnf);

/**
 * @brief  Ghi trang thai logic ra chan GPIO (Ghi nguyen tu qua thanh ghi BSRR/BRR)
 * @param  port: Port GPIO
 * @param  pin:  Chi so chan (0 .. 15)
 * @param  state: 1 = HIGH (Set), 0 = LOW (Reset)
 */
void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t state);

/**
 * @brief  Doc trang thai logic hien tai cua mot chan GPIO tu thanh ghi IDR
 * @param  port: Port GPIO
 * @param  pin:  Chi so chan (0 .. 15)
 * @retval 1 = HIGH, 0 = LOW
 */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin);

/**
 * @brief  Dao trang thai logic cua mot chan GPIO
 * @param  port: Port GPIO
 * @param  pin:  Chi so chan (0 .. 15)
 */
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin);

/**
 * @brief  Khoi tao ngat ngoai EXTI5 tren chan PA5 (Ket noi chan tin hieu OS cua LM75)
 */
void exti5_init(void);

#endif /* GPIO_DRIVER_H_ */
