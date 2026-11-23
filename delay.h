#ifndef DELAY_H_
#define DELAY_H_

#include <stdint.h>

void delay_init(void);
void delay_us(uint16_t us);
void delay(uint16_t ms);

#endif /* DELAY_H_ */
