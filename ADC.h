#ifndef __ADC_H
#define __ADC_H

#include "stm32f10x.h"
#include <stdint.h>

void     ADC_Config(void);            // C?u hình ADC1
uint16_t ADC_Read(uint8_t channel);   // Ð?c 1 l?n ADC kênh b?t k? (0–15)

#endif
