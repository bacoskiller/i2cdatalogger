/**
 * @file    eeprom_i2c.h
 * @brief   Trinh dieu khien giao tiep EEPROM I2C (AT24C256C / 24C32)
 * @details Cung cap cac ham ghi/doc 1 byte, 2 byte va chuoi du lieu theo ranh gioi trang.
 */

#ifndef EEPROM_I2C_H_
#define EEPROM_I2C_H_

#include <stdint.h>
#include "i2c_driver.h"

/* Dia chi I2C 7-bit cua EEPROM (A2 = A1 = A0 = 0 -> 0x50) */
#define EEPROM_I2C_ADDR        0x50

/* Thong so chip AT24C256C: 32 Kbyte (256 Kbit), kich thuoc trang 64 byte */
#define EEPROM_PAGE_SIZE       64
#define EEPROM_SIZE_BYTES      32768

/**
 * @brief  Ghi mot mang du lieu vao EEPROM (Toi da 8 byte cho moi lan goi ham de dam bao an toan ranh gioi trang)
 * @param  mem_addr: Dia chi bo nho 16-bit can ghi
 * @param  data: Con tro den vung du lieu can ghi
 * @param  length: So luong byte can ghi (1..8 byte)
 * @retval I2C_OK neu thanh cong, hoac ma loi I2C tuong ung
 */
i2c_status_t eeprom_write_buffer(uint16_t mem_addr, uint8_t *data, uint8_t length);

/**
 * @brief  Ghi 1 byte du lieu vao EEPROM
 * @param  mem_addr: Dia chi bo nho 16-bit
 * @param  data: Byte du lieu muon ghi
 * @retval I2C_OK hoac ma loi I2C
 */
i2c_status_t eeprom_write_data(uint16_t mem_addr, uint8_t data);

/**
 * @brief  Doc 1 byte du lieu tu dia chi bat ky (Random Read)
 * @param  mem_addr: Dia chi bo nho 16-bit can doc
 * @param  data: Con tro luu byte du lieu nhan ve
 * @retval I2C_OK hoac ma loi I2C
 */
i2c_status_t eeprom_read_byte(uint16_t mem_addr, uint8_t *data);

/**
 * @brief  Doc 2 byte lien tiep (dinh dang Big-Endian)
 * @param  mem_addr: Dia chi bo nho byte cao
 * @param  value: Con tro luu gia tri 16-bit nhan ve
 * @retval I2C_OK hoac ma loi I2C
 */
i2c_status_t eeprom_read_word(uint16_t mem_addr, uint16_t *value);

/**
 * @brief  Doc mot khoi du lieu lien tuc (Sequential Read) tu EEPROM
 * @param  mem_addr: Dia chi bat dau doc
 * @param  data: Con tro vung dem nhan du lieu
 * @param  length: So luong byte can doc
 * @retval I2C_OK hoac ma loi I2C
 */
i2c_status_t eeprom_read_buffer(uint16_t mem_addr, uint8_t *data, uint8_t length);

#endif /* EEPROM_I2C_H_ */
