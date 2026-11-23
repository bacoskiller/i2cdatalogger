/**
 * @file    i2c_driver.h
 * @brief   Trinh dieu khien giao tiep I2C Bare-metal cap thanh ghi cho STM32F103
 * @details Ho tro Master Transmitter, Master Receiver (1-byte, 2-byte, Sequential Read)
 *          va thuat toan giai phong bus ket (9 xung SCL theo chuan NXP UM10204).
 */

#ifndef I2C_DRIVER_H_
#define I2C_DRIVER_H_

#include <stdint.h>

#ifndef I2C_MODE_MASTER
#define I2C_MODE_MASTER  0
#define I2C_MODE_SLAVE   1
#endif

/* Ma trang thai tra ve cua cac ham I2C */
typedef enum {
    I2C_OK          =  0,   // Giao dich I2C thanh cong
    I2C_ERR_TIMEOUT = -1,   // Qua thoi gian cho co trang thai (chong treo he thong)
    I2C_ERR_NACK    = -2,   // Thiet bi to phan hoi NACK (sai dia chi hoac dang ban)
    I2C_ERR_BUS     = -3    // Loi phan cung bus: BERR, ARLO hoac duong truyen bi ket
} i2c_status_t;

/**
 * @brief  Khoi tao ngoai vi I2C1 (PB6=SCL, PB7=SDA, 100 kHz Standard-mode)
 */
void i2c_init(void);

/**
 * @brief  Thuật toan giai phong bus khi bi ket (Phat 9 xung clock SCL theo NXP UM10204)
 */
void i2c_recover(void);

/**
 * @brief  Cho cho den khi bus ranh (Co BUSY = 0 tren I2C1->SR2)
 */
i2c_status_t i2c_wait_bus_free(void);

/* --- Cac ham giao thuc I2C cap thap --- */
i2c_status_t i2c_start(uint8_t mode);
void         i2c_stop(void);
i2c_status_t i2c_send_address(uint8_t address, uint8_t rw);
i2c_status_t i2c_write_data(uint8_t data);

/* --- API truyen nhan du lieu thong dung --- */
i2c_status_t i2c_write_byte(uint8_t slave_address, uint8_t data);
i2c_status_t i2c_write_mul_bytes(uint8_t slave_address, uint8_t *data, uint8_t length);

/**
 * @brief  Doc 1 hoac 2 byte tu thiet bi to theo chuan ST RM0008 (dung bit POS va BTF)
 */
i2c_status_t i2c_read_mul_bytes(uint8_t slave_address, uint8_t *data, uint8_t length);

/**
 * @brief  Doc chuoi nhieu byte lien tuc (Sequential Read)
 */
i2c_status_t i2c_read_buffer(uint8_t slave_address, uint8_t *data, uint8_t length);

/**
 * @brief  Ghi wlen byte roi Repeated START doc rlen byte (1 hoac 2 byte)
 */
i2c_status_t i2c_write_read(uint8_t slave_address, const uint8_t *wdata, uint8_t wlen,
                            uint8_t *rdata, uint8_t rlen);

#endif /* I2C_DRIVER_H_ */
