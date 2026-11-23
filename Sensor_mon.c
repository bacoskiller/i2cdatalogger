/**
 * @file    Sensor_mon.c
 * @brief   Module giam sat suc khoe cam bien nhiet do LM75 (Sensor Monitor)
 * @details Xu ly tu dong khoi phuc cau hinh Tos/Thyst sau mat nguon, loc rung nhieu
 *          va doi chieu tin hieu phan cung chan OS voi du lieu so I2C.
 */

#include "Sensor_mon.h"

static lm75_status_t fault      = LM75_OK; /**< Ma trang thai loi hien tai cua cam bien */
static uint8_t       configured = 0;       /**< Co danh dau da ghi va xac minh Tos/Thyst thanh cong */
static uint8_t       fail_cnt   = 0;       /**< Bo dem so lan doc that bai lien tiep */
static uint8_t       ok_cnt     = 0;       /**< Bo dem so lan doc thanh cong lien tiep khi dang phuc hoi */

/**
 * @brief  Reset bo giam sat ve trang thai ban dau
 */
void sensor_mon_reset(void)
{
    fault      = LM75_OK;
    configured = 0;
    fail_cnt   = 0;
    ok_cnt     = 0;
    LM75_ResetFilter();
}

/**
 * @brief  Lay ma loi hien tai cua cam bien
 */
lm75_status_t sensor_mon_fault(void)
{
    return fault;
}

/**
 * @brief  Kiem tra cheo giua gia tri nhiet do va muc logic phan cung chan OS
 * @details Chan OS cua LM75 hoat dong o che do Comparator Active-LOW:
 *          - Khi nhiet do t >= Tos (40 do C): OS bi keo xuong LOW (0).
 *          - Khi nhiet do t <= Thyst (39 do C): OS duoc tha ve muc HIGH (1).
 *          Neu phat hien su bat nhat vuot qua bien do lech an toan, bao loi bat thuong.
 */
static uint8_t os_matches(int16_t t, uint8_t os_low)
{
#if OS_CROSSCHECK
    if (t >= (TEMP_TOS_C   * 10 + OS_MARGIN_X10) && !os_low) return 0;
    if (t <= (TEMP_THYST_C * 10 - OS_MARGIN_X10) &&  os_low) return 0;
#else
    (void)t;
    (void)os_low;
#endif
    return 1;
}

/**
 * @brief  Ham phuc vu giam sat va doc nhiet do cam bien dinh ky
 */
uint8_t sensor_mon_service(uint8_t want_sample, uint8_t os_low, int16_t *t)
{
    lm75_status_t st;
    uint8_t got = 0;

    if (!configured)
    {
        /* Cam bien chua duoc cau hinh (moi khoi dong, hoac vua bi mat nguon phuc hoi lai):
         * Tien hanh ghi lai nguong Tos va Thyst. Neu mat nguon, LM75 se bi reset ve mac dinh
         * (Tos = 80 do C, Thyst = 75 do C), nen viec khoi tao lai la bat buoc. */
        st = LM75_Init(TEMP_TOS_C, TEMP_THYST_C);
        if (st == LM75_OK)
        {
            configured = 1;
            LM75_ResetFilter();
        }
    }
    else if (want_sample || fault != LM75_OK)
    {
        /* Doc gia tri nhiet do thuc te qua I2C */
        st = LM75_ReadTemp(t);
        if (st == LM75_OK && !os_matches(*t, os_low))
        {
            st = LM75_ERR_BAD_VALUE; /* Muc logic chan OS khong khop voi gia tri nhiet do */
        }

        if (st == LM75_OK)
        {
            got = 1;
        }
    }
    else
    {
        return 0; /* Khong co yeu cau doc mau o trang thai hien tai */
    }

    /* Quan ly bo dem loi va co che tu dong phuc hoi */
    if (st == LM75_OK)
    {
        fail_cnt = 0;
        /* Neu dang o trang thai co loi truoc do, can du SENSOR_RECOVER_OK lan thanh cong de xoa loi */
        if (fault != LM75_OK && got && ++ok_cnt >= SENSOR_RECOVER_OK)
        {
            fault  = LM75_OK;            /* Da phuc hoi hoat dong binh thuong */
            ok_cnt = 0;
        }
    }
    else
    {
        ok_cnt = 0;
        if (fail_cnt < 255) fail_cnt++;

        /* Xac nhan cam bien hong neu loi vuot qua gioi han SENSOR_FAIL_LIMIT */
        if (fail_cnt >= SENSOR_FAIL_LIMIT)
        {
            if (fault == LM75_OK)
            {
                LM75_ResetFilter();      /* Xoa bo dem loc nhay cho lan doc phuc hoi dau tien */
            }
            fault      = st;             /* Cap nhat ma loi chi tiet */
            configured = 0;              /* Danh dau can phai cau hinh lai khi phuc hoi */
        }
    }

    return got;
}
