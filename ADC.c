#include "ADC.h"

// Cấu hình ADC1, chạy single conversion, kích SWSTART
void ADC_Config(void)
{
    // Bật clock cho ADC1
    RCC->APB2ENR |= (1 << 9);

    // Chọn prescaler cho ADC
    RCC->CFGR &= ~(3 << 14);   // Xoá 2 bit ADCPRE
    RCC->CFGR |=  (2 << 14);   // 10b: PCLK2 / 6

    // Reset các thanh ghi CR1, CR2 
    ADC1->CR1 = 0;
    ADC1->CR2 = 0;

    // Căn phải dữ liệu, single conversion
		ADC1->CR2 &= ~(1 << 11);   // ALIGN = 0: dữ liệu căn phải
    ADC1->CR2 &= ~(1 << 1);    // CONT  = 0: chế độ single conversion

    // Cho phép trigger bằng SWSTART
    ADC1->CR2 |= (7 << 17);   // EXTSEL = 111: SWSTART
    ADC1->CR2 |= (1 << 20);   // EXTTRIG enable

    // Bật ADC và calib 
    ADC1->CR2 |= (1 << 0);    // ADON = 1 (bật ADC)

    // Reset calibration
    ADC1->CR2 |= (1 << 3);          // RSTCAL = 1
    while (ADC1->CR2 & (1 << 3));   // Chờ RSTCAL = 0

    // Bắt đầu calibration
    ADC1->CR2 |= (1 << 2);          // CAL = 1
    while (ADC1->CR2 & (1 << 2));   // Chờ CAL = 0 (xong)
}

uint16_t ADC_Read(uint8_t channel)
{
    // Cấu hình thời gian lấy mẫu (sample time) cho kênh đó
    if (channel <= 9)
    {
        ADC1->SMPR2 &= ~(7 << (channel * 3));   // xoá
        ADC1->SMPR2 |=  (7 << (channel * 3));   // 239.5 cycles
    }
    else
    {
        uint8_t ch = channel - 10;
        ADC1->SMPR1 &= ~(7 << (ch * 3));
        ADC1->SMPR1 |=  (7 << (ch * 3));
    }

    // Cấu hình dãy chuyển đổi (regular sequence)
    ADC1->SQR1 &= ~(0xF << 20);      	// L = 0 -> 1 conversion
    ADC1->SQR3 &= ~0x1F;            	// xoá kênh cũ
    ADC1->SQR3 |= (channel & 0x1F); 	// chọn kênh mới

    // Bật ADC nếu chưa bật 
    ADC1->CR2 |= (1 << 0);          	// ADON = 1

    // Bắt đầu chuyển đổi bằng SWSTART 
    ADC1->CR2 |= (1 << 22);         	// SWSTART = 1

    // Chờ EOC (End Of Conversion) 
    while (!(ADC1->SR & (1 << 1))); 	// EOC bit1

    // Đọc dữ liệu 12-bit (0..4095) 
    return (uint16_t)ADC1->DR;
}
