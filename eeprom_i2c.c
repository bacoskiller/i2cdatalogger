/**
 * @file    eeprom_i2c.c
 * @brief   Trinh dieu khien giao tiep EEPROM I2C (AT24C256C / 24C32)
 * @details Ho tro truy xuat bo nho 16-bit, ghi theo page buffer va doc tuan tu.
 */

#include "stm32f10x.h"
#include "i2c_driver.h"
#include "eeprom_i2c.h"
#include "delay.h"

/**
 * @brief  Ghi mang du lieu toi da 8 byte vao EEPROM (Page Write)
 * @details Dong goi 2 byte dia chi bo nho 16-bit (MSB, LSB) va du lieu vao mot khung truyen I2C.
 *          Sau khi truyen, can cho it nhat tWR (5ms) de chip hoan thanh chu trinh ghi noi bo.
 */
i2c_status_t eeprom_write_buffer(uint16_t mem_addr, uint8_t *data, uint8_t length)
{
    if (length > 8 || data == 0) {
        return I2C_ERR_BUS; // Chan tran bo dem va tham so khong hop le
    }

    uint8_t buf[10]; // 2 byte dia chi + toi da 8 byte du lieu
    buf[0] = (uint8_t)(mem_addr >> 8);   // Byte dia chi cao (MSB)
    buf[1] = (uint8_t)(mem_addr & 0xFF); // Byte dia chi thap (LSB)
    for (uint8_t i = 0; i < length; i++) {
        buf[2 + i] = data[i];
    }
    
    i2c_status_t st = i2c_write_mul_bytes(EEPROM_I2C_ADDR, buf, length + 2);
    delay(10); // Cho chu trinh ghi vat ly cua EEPROM hoan tat (tWR <= 5ms tren AT24C256C)
    return st;
}

/**
 * @brief  Ghi 1 byte du lieu vao dia chi bo nho
 */
i2c_status_t eeprom_write_data(uint16_t mem_addr, uint8_t data)
{
    return eeprom_write_buffer(mem_addr, &data, 1);
}

/**
 * @brief  Doc 1 byte du lieu (Random Read)
 * @details Gui 2 byte dia chi de thiet lap con tro noi bo, sau do doc 1 byte du lieu tu bus.
 */
i2c_status_t eeprom_read_byte(uint16_t mem_addr, uint8_t *data)
{
    uint8_t addr[2];
    addr[0] = (uint8_t)(mem_addr >> 8);
    addr[1] = (uint8_t)(mem_addr & 0xFF);
    
    // Thiet lap con tro dia chi noi bo
    i2c_status_t st = i2c_write_mul_bytes(EEPROM_I2C_ADDR, addr, 2);
    if (st != I2C_OK) return st;
    delay(2);
    
    // Doc 1 byte tai vi tri con tro
    return i2c_read_mul_bytes(EEPROM_I2C_ADDR, data, 1);
}

/**
 * @brief  Doc 2 byte lien tiep theo thu tu Big-Endian
 */
i2c_status_t eeprom_read_word(uint16_t mem_addr, uint16_t *value)
{
    uint8_t b1 = 0, b2 = 0;
    i2c_status_t st = eeprom_read_byte(mem_addr, &b1);
    if (st != I2C_OK) return st;
    delay(2);
    st = eeprom_read_byte(mem_addr + 1, &b2);
    if (st == I2C_OK && value != 0) {
        *value = (uint16_t)(((uint16_t)b1 << 8) | b2);
    }
    return st;
}

/**
 * @brief  Doc chuoi length byte lien tuc (Sequential Read)
 * @details Thiet lap con tro dia chi bat dau (Dummy Write), sau do nhan lien tuc length byte
 *          giup toi uu hoa bang thong I2C so voi doc tung byte rieng le.
 */
i2c_status_t eeprom_read_buffer(uint16_t mem_addr, uint8_t *data, uint8_t length)
{
    if (data == 0 || length == 0) return I2C_ERR_BUS;

    uint8_t addr[2];
    addr[0] = (uint8_t)(mem_addr >> 8);
    addr[1] = (uint8_t)(mem_addr & 0xFF);

    // Thiet lap con tro dia chi bat dau
    i2c_status_t st = i2c_write_mul_bytes(EEPROM_I2C_ADDR, addr, 2);
    if (st != I2C_OK) return st;
    delay(2);

    // Doc stream lien tiep length byte
    return i2c_read_buffer(EEPROM_I2C_ADDR, data, length);
}
