/**
 * @file    Sensor_mon.h
 * @brief   Module giam sat suc khoe cam bien nhiet do LM75 (Sensor Monitor)
 * @details Quan ly trang thai loi, tu dong khoi tao lai khi mat nguon, kiem tra cheo
 *          muc logic chan OS voi gia tri so doc duoc qua giao thuc I2C.
 */

#ifndef SENSOR_MON_H_
#define SENSOR_MON_H_

#include <stdint.h>
#include "lm75.h"

/* Cac nguong nhiet do bao dong va tre (Hysteresis) */
#ifndef TEMP_TOS_C
#define TEMP_TOS_C          40      /**< Nguong bao dong qua nhiet (40 do C) */
#endif

#ifndef TEMP_THYST_C
#define TEMP_THYST_C        39      /**< Nguong giai toa bao dong (39 do C) */
#endif

/* Thong so bo loc loi va tu phuc hoi */
#define SENSOR_FAIL_LIMIT   3       /**< So lan doc loi lien tiep de xac nhan cam bien bi hong/mat ket noi */
#define SENSOR_RECOVER_OK   2       /**< So lan doc tot lien tiep de xac nhan cam bien da phuc hoi */
#define OS_CROSSCHECK       1       /**< 1: Bat kiem tra cheo chan OS voi nhiet do I2C, 0: Bo qua */
#define OS_MARGIN_X10       10      /**< Du dia an toan 1.0 do C quanh bien nguong Tos va Thyst */

/**
 * @brief  Khoi tao lai toan bo bo giam sat cam bien (Goi khi bat nguon hoac reset he thong)
 */
void sensor_mon_reset(void);

/**
 * @brief  Lay ma trang thai loi hien tai cua cam bien
 * @retval LM75_OK neu cam bien hoat dong binh thuong, nguoc lai la ma loi LM75
 */
lm75_status_t sensor_mon_fault(void);

/**
 * @brief  Ham phuc vu giam sat chu ky cua cam bien LM75
 * @param  want_sample: 1 khi o che do can doc nhiet do (RUN), 0 khi o che do nghi
 * @param  os_low: 1 neu chan PA5 (OS) dang o muc LOW, 0 neu o muc HIGH
 * @param  t: Con tro luu gia tri nhiet do do duoc (*10)
 * @retval 1 neu vua thu thap duoc mau nhiet do hop le moi, 0 neu chua co mau hoac gap loi
 */
uint8_t sensor_mon_service(uint8_t want_sample, uint8_t os_low, int16_t *t);

#endif /* SENSOR_MON_H_ */
