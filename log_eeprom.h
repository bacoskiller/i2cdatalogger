/**
 * @file    log_eeprom.h
 * @brief   Module quan ly nhat ky du lieu tren EEPROM khong dung Header (Zero-Header Wear Leveling)
 * @details Moi ban ghi chiem 8 byte, khop voi ranh gioi trang cua AT24C256C (64 byte/trang).
 *          Thuat toan quet Magic Bytes (0xAA 0x55) va Timestamp giup tu dong khoi phuc vi tri ghi
 *          sau khi mat nguon ma khong can ghi de Header len Trang 0.
 */

#ifndef LOG_EEPROM_H_
#define LOG_EEPROM_H_

#include <stdint.h>
#include "i2c_driver.h"

/**
 * @brief Cau truc mot ban ghi nhat ky nhiet do tren RAM
 */
typedef struct {
    uint32_t timestamp;     /**< Thoi diem do (giay) */
    int16_t  temperature;   /**< Gia tri nhiet do * 10 (vi du: 255 = 25.5 do C) */
    uint16_t reserved;      /**< Du phong / Magic bytes */
} LogRecord_t;

/**
 * @brief  Khoi dong he thong luu tru EEPROM
 * @details Quet cac slot de xac dinh so luong ban ghi hop le va con tro ghi tiep theo.
 */
void log_init(void);

/**
 * @brief  Ghi mot ban ghi nhat ky moi vao EEPROM theo co che bo dem vong (Circular Buffer)
 * @param  timestamp: Thoi diem ghi nhan (giay)
 * @param  temp_x10: Gia tri nhiet do da nhan 10
 * @retval I2C_OK neu ghi thanh cong, hoac ma loi I2C
 */
i2c_status_t log_append(uint32_t timestamp, int16_t temp_x10);

/**
 * @brief  Doc mot ban ghi theo thu tu nguoc thoi gian
 * @param  back_index: 0 la ban ghi moi nhat, 1 la ke truoc do, ...
 * @param  record: Con tro luu ket qua giai ma
 * @retval 1 neu doc thanh cong va dung Magic Bytes, 0 neu khong hop le hoac loi I2C
 */
uint8_t log_get(uint16_t back_index, LogRecord_t *record);

/**
 * @brief  Xoa toan bo nhat ky (Pha huy Magic Bytes cua cac slot tren EEPROM)
 * @retval I2C_OK neu hoan tat
 */
i2c_status_t log_clear(void);

/**
 * @brief  Lay tong so ban ghi hop le hien co trong he thong (Toi da 50)
 * @retval So luong ban ghi
 */
uint16_t log_count(void);

/**
 * @brief  Lay chi so slot tiep theo se duoc ghi
 * @retval Vi tri slot (0 .. 49)
 */
uint16_t log_head(void);

#endif /* LOG_EEPROM_H_ */
