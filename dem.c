/**
 * @file    dem.c
 * @brief   Hien thuc May trang thai FSM dieu phoi he thong va quan ly ngat
 * @details Quan ly 4 trang thai: OFF, RUNNING, VIEW_LOG, ALARM.
 *          Tich hop SysTick Timer tao moc thoi gian doc lap va co che
 *          chong chop giat man hinh LCD bang co prev_state.
 */

#include "stm32f10x.h"
#include "gpio_driver.h"
#include "lcd_i2c.h"
#include "delay.h"
#include "dem.h"
#include "lm75.h"
#include "log_eeprom.h"
#include <stdint.h>

/* ====================================================================
 * DINH NGHIA TRANG THAI VA BIEN HE THONG
 * ==================================================================== */
typedef enum {
    STATE_OFF = 0,      // He thong o che do cho
    STATE_RUNNING,      // Che do do, luu nhat ky va giam sat
    STATE_VIEW_LOG,     // Che do xem lai lich su ban ghi
    STATE_ALARM         // Che do bao dong qua nhiet
} SystemState_t;

volatile SystemState_t sys_state = STATE_OFF;
volatile int8_t prev_state = -1; // -1: Danh dau khoi tao lan dau de ve man hinh

/* Cac co bao ngat tu nut nhan va chan bao dong OS */
volatile uint8_t flag_btn_start = 0;
volatile uint8_t flag_btn_stop  = 0;
volatile uint8_t flag_btn_read  = 0;
volatile uint8_t flag_btn_del   = 0;
volatile uint8_t flag_os_alarm  = 0;

/* Bo dem thoi gian thuc he thong (Epoch Timestamp tu ngat SysTick) */
volatile uint32_t system_time = 1700000000; 
uint32_t last_save_time = 0;

static uint8_t view_idx = 0; // Con tro vi tri khi duyet nhat ky trong STATE_VIEW_LOG

/* ====================================================================
 * CAU HINH SYSTICK TIMER (CORTEX-M3 BARE-METAL)
 * ==================================================================== */

/**
 * @brief  Khoi tao SysTick Timer tao ngat chu ky 100ms (10 Hz)
 *         SystemCoreClock = 72 MHz -> 7,200,000 xung nhip moi tick.
 */
void systick_init(void)
{
    SysTick->LOAD = (SystemCoreClock / 10) - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
                    SysTick_CTRL_TICKINT_Msk   | 
                    SysTick_CTRL_ENABLE_Msk;
}

/**
 * @brief  Trinh phuc vu ngat SysTick Handler
 *         Dem du 10 lan ngat 100ms se tang system_time len 1 giay vat ly.
 */
void SysTick_Handler(void)
{
    static uint8_t tick_100ms_cnt = 0;
    tick_100ms_cnt++;
    if (tick_100ms_cnt >= 10) 
    {
        tick_100ms_cnt = 0;
        system_time++; // Tang 1 giay thoi gian thuc (khong phu thuoc vong lap delay)
    }
}

/* ====================================================================
 * CAC HAM TIEN ICH DINH DANG VA HIEN THI LCD
 * ==================================================================== */

/**
 * @brief  Dinh dang nhiet do co dau sang chuoi 6 ky tu dang " 25.5" hoac "-05.5"
 */
void format_temp_signed(int16_t value, char *buf) 
{
    uint8_t neg = (value < 0);
    uint16_t v = neg ? (uint16_t)(-value) : (uint16_t)value;
    
    buf[0] = neg ? '-' : ' ';
    if (v >= 1000) {
        buf[1] = '0' + (v / 1000) % 10;
    } else {
        buf[1] = ' ';
    }
    buf[2] = '0' + (v / 100) % 10;
    buf[3] = '0' + (v / 10) % 10;
    buf[4] = '.';
    buf[5] = '0' + (v % 10);
    buf[6] = '\0';
}

/**
 * @brief  Hien thi nhan va gia tri nhiet do len mot dong cua LCD
 */
static void lcd_show_temp(uint8_t row, const char *label, int16_t value) 
{
    char buf[7];
    format_temp_signed(value, buf);
    set_cursor(0, row);
    lcd_send_string((char *)label);
    set_cursor(6, row);
    lcd_send_string(buf);
    lcd_send_string(" C  ");
}

/* ====================================================================
 * KHOI TAO NGAT NGOAI EXTI (CANH XUONG - FALLING EDGE CHO NUT NHAN)
 * ==================================================================== */

void exti0_init(void) 
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->EXTICR[0] &= ~(0xF << 0);     // PA0 -> EXTI0
    EXTI->PR   = (1 << 0);              // Xoa co pending (rc_w1)
    EXTI->IMR  |= (1 << 0);             // Cho phep ngat line 0
    EXTI->FTSR |= (1 << 0);             // Ngat suon xuong khi bam nut
    EXTI->RTSR &= ~(1 << 0);            // Khong dung suon len
    NVIC_EnableIRQ(EXTI0_IRQn);
}

void EXTI0_IRQHandler(void) 
{
    if (EXTI->PR & (1 << 0)) { 
        EXTI->PR = (1 << 0);            // Xoa co ngat
        flag_btn_start = 1; 
    }
}

void exti1_init(void) 
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->EXTICR[0] &= ~(0xF << 4);     // PA1 -> EXTI1
    EXTI->PR   = (1 << 1);
    EXTI->IMR  |= (1 << 1);
    EXTI->FTSR |= (1 << 1);
    EXTI->RTSR &= ~(1 << 1);
    NVIC_EnableIRQ(EXTI1_IRQn);
}

void EXTI1_IRQHandler(void) 
{
    if (EXTI->PR & (1 << 1)) { 
        EXTI->PR = (1 << 1); 
        flag_btn_stop = 1; 
    }
}

void exti2_init(void) 
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->EXTICR[0] &= ~(0xF << 8);     // PA2 -> EXTI2
    EXTI->PR   = (1 << 2);
    EXTI->IMR  |= (1 << 2);
    EXTI->FTSR |= (1 << 2);
    EXTI->RTSR &= ~(1 << 2);
    NVIC_EnableIRQ(EXTI2_IRQn);
}

void EXTI2_IRQHandler(void) 
{
    if (EXTI->PR & (1 << 2)) { 
        EXTI->PR = (1 << 2); 
        flag_btn_read = 1; 
    }
}

void exti3_init(void) 
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO->EXTICR[0] &= ~(0xF << 12);    // PA3 -> EXTI3
    EXTI->PR   = (1 << 3);
    EXTI->IMR  |= (1 << 3);
    EXTI->FTSR |= (1 << 3);
    EXTI->RTSR &= ~(1 << 3);
    NVIC_EnableIRQ(EXTI3_IRQn);
}

void EXTI3_IRQHandler(void) 
{
    if (EXTI->PR & (1 << 3)) { 
        EXTI->PR = (1 << 3); 
        flag_btn_del = 1; 
    }
}

void EXTI9_5_IRQHandler(void) 
{
    // PA5 - Chan bao dong OS cua cam bien nhiet do LM75
    if (EXTI->PR & (1 << 5)) { 
        EXTI->PR = (1 << 5); 
        flag_os_alarm = 1; 
    }
}

/* ====================================================================
 * CHUONG TRINH DIEU PHOI MAY TRANG THAI CHINH (FSM)
 * ==================================================================== */
void dem_sp(void)
{
    switch(sys_state) 
    {
        /* ------------------------------------------------------------
         * TRANG THAI 1: STATE_OFF (HE THONG TAM DUNG)
         * ------------------------------------------------------------ */
        case STATE_OFF:
            // Cap nhat giao dien khi vua chuyen trang thai
            if(prev_state != STATE_OFF) {
                lcd_display_on();
                lcd_clear();
                set_cursor(0, 0);
                lcd_send_string("HE THONG: OFF   ");
                set_cursor(0, 1);
                lcd_send_string("PA0: BAM START  ");
                prev_state = STATE_OFF;
            }

            // Nhan nut START (PA0) de khoi dong he thong
            if(flag_btn_start || gpio_read(GPIOA, 0) == 0) {
                flag_btn_start = 0;
                lcd_clear();
                set_cursor(0, 0); 
                lcd_send_string("Dang khoi dong..");
                set_cursor(0, 1);
                lcd_send_string("Doc cam bien... ");
                delay(100);

                // Cau hinh nguong canh bao LM75: Tos = 40.0 C, Thyst = 39.0 C
                LM75_Init(40, 39); 
                delay(50);
                
                // Khoi phuc vi tri ghi nhat ky EEPROM tu Magic Bytes
                log_init();
                delay(200);

                last_save_time = system_time;
                sys_state = STATE_RUNNING;
            }
            delay(100);
            break;
            
        /* ------------------------------------------------------------
         * TRANG THAI 2: STATE_RUNNING (DO NHIET DO VA GHI NHAT KY)
         * ------------------------------------------------------------ */
        case STATE_RUNNING:
            if(prev_state != STATE_RUNNING) {
                lcd_clear();
                prev_state = STATE_RUNNING;
            }
            
            // Uu tien 1: Nut STOP (PA1) chuyen ve OFF
            if(flag_btn_stop || gpio_read(GPIOA, 1) == 0) {
                flag_btn_stop = 0;
                sys_state = STATE_OFF;
                break;
            }

            // Uu tien 2: Canh bao qua nhiet tu chan phan cung OS (PA5 Active-LOW)
            if(flag_os_alarm || gpio_read(GPIOA, 5) == 0) {
                sys_state = STATE_ALARM;
                break;
            }
            
            // Uu tien 3: Nhan nut READ (PA2) chuyen sang che do xem lich su log
            if(flag_btn_read || gpio_read(GPIOA, 2) == 0) {
                flag_btn_read = 0;
                view_idx = 0;
                sys_state = STATE_VIEW_LOG;
                break;
            }
            
            // Uu tien 4: Nhan nut CLEAR (PA3) xoa toan bo nhat ky EEPROM
            if(flag_btn_del || gpio_read(GPIOA, 3) == 0) {
                flag_btn_del = 0;
                log_clear();
                lcd_clear();
                set_cursor(0, 0); 
                lcd_send_string("DA XOA EEPROM!  ");
                set_cursor(0, 1);
                lcd_send_string("RESET LOG = 00  ");
                delay(600);
                lcd_clear();
            }

            // Uu tien 5: Doc cam bien va cap nhat hien thi LCD
            int16_t t;
            if(LM75_ReadTemp(&t) == LM75_OK) {
                lcd_show_temp(0, "TEMP:", t);
                
                // Bao ve 2 tang: Kiem tra nhiet do phan mem vuot nguong 40.0 C
                if (t >= 400) {
                    flag_os_alarm = 1;
                    sys_state = STATE_ALARM;
                    break;
                }

                // Dong 2: Hien thi trang thai RUN va so luong ban ghi hien tai
                char line2[17];
                uint16_t c = log_count();
                line2[0]  = 'R'; line2[1]  = 'U'; line2[2]  = 'N'; line2[3]  = ' '; 
                line2[4]  = ' '; line2[5]  = 'L'; line2[6]  = 'O'; line2[7]  = 'G';
                line2[8]  = ':'; line2[9]  = ' ';
                line2[10] = '0' + (c / 10) % 10;
                line2[11] = '0' + (c % 10);
                line2[12] = '/'; line2[13] = '5'; line2[14] = '0'; line2[15] = ' ';
                line2[16] = '\0';
                set_cursor(0, 1);
                lcd_send_string(line2);

                // Ghi dinh ky vao EEPROM moi 5 giay (True Wear-Leveling 8-byte)
                if(system_time - last_save_time >= 5) {
                    log_append(system_time, t);
                    last_save_time = system_time;
                }
            } else {
                set_cursor(0, 0);
                lcd_send_string("TEMP: --.- C    ");
                set_cursor(0, 1);
                lcd_send_string("RUN (SENSOR ERR)");
            }
            delay(100);
            break;
            
        /* ------------------------------------------------------------
         * TRANG THAI 3: STATE_VIEW_LOG (XEM LAI LICH SU NHAT KY)
         * ------------------------------------------------------------ */
        case STATE_VIEW_LOG:
            if(prev_state != STATE_VIEW_LOG) {
                lcd_clear();
                prev_state = STATE_VIEW_LOG;
                view_idx = 0;

                if(log_count() == 0) {
                    set_cursor(0, 0);
                    lcd_send_string("CHUA CO LICH SU!");
                    set_cursor(0, 1);
                    lcd_send_string("Luu sau moi 5s..");
                    delay(500);
                    sys_state = STATE_RUNNING;
                    break;
                } else {
                    flag_btn_read = 1; // Tu dong hien thi ban ghi dau tien
                }
            }
            
            // Nut STOP luon co quyen thoat ve STATE_OFF
            if(flag_btn_stop || gpio_read(GPIOA, 1) == 0) {
                flag_btn_stop = 0;
                sys_state = STATE_OFF;
                break;
            }

            // Nhan nut READ (PA2) de chuyen sang ban ghi tiep theo
            if(flag_btn_read || gpio_read(GPIOA, 2) == 0) {
                flag_btn_read = 0;
                
                LogRecord_t view_rec;
                uint16_t total = log_count();
                if(view_idx < total && log_get(view_idx, &view_rec)) {
                    lcd_clear();
                    char buf[17];
                    // Dong 1: LOG 01/05
                    buf[0] = 'L'; buf[1] = 'O'; buf[2] = 'G'; buf[3] = ' ';
                    buf[4] = '0' + (view_idx + 1) / 10;
                    buf[5] = '0' + (view_idx + 1) % 10;
                    buf[6] = '/';
                    buf[7] = '0' + (total / 10);
                    buf[8] = '0' + (total % 10);
                    buf[9] = '\0';
                    set_cursor(0, 0); 
                    lcd_send_string(buf);
                    
                    // Dong 2: Hien thi nhiet do ban ghi tuong ung
                    lcd_show_temp(1, "VAL:", view_rec.temperature);
                    view_idx++;
                    delay(250); // Chong doi phim khi duyet lich su
                } else {
                    // Da duyet het toan bo danh sach
                    lcd_clear();
                    set_cursor(0, 0);
                    lcd_send_string("HET LICH SU!    ");
                    set_cursor(0, 1);
                    lcd_send_string("Tro ve RUN...   ");
                    delay(500);
                    sys_state = STATE_RUNNING;
                }
            }
            
            // Canh bao qua nhiet van duoc giam sat ngam
            if(flag_os_alarm || gpio_read(GPIOA, 5) == 0) {
                sys_state = STATE_ALARM;
            }
            delay(100);
            break;
            
        /* ------------------------------------------------------------
         * TRANG THAI 4: STATE_ALARM (BAO DONG QUA NHIET)
         * ------------------------------------------------------------ */
        case STATE_ALARM:
            if(prev_state != STATE_ALARM) {
                lcd_clear();
                prev_state = STATE_ALARM;
            }
            
            // Nut STOP luon co quyen thoat an toan ve OFF
            if(flag_btn_stop || gpio_read(GPIOA, 1) == 0) {
                flag_btn_stop = 0;
                flag_os_alarm = 0;
                sys_state = STATE_OFF;
                break;
            }

            // Hieu ung chop tat dong chu canh bao tren dong 1
            static uint8_t blink = 0;
            if(blink) {
                set_cursor(0, 0); lcd_send_string("! CANH BAO !    ");
            } else {
                set_cursor(0, 0); lcd_send_string("! QUA NHIET !   ");
            }
            blink = !blink;
            
            // Doc nhiet do thoi gian thuc tren dong 2
            int16_t al_temp;
            if(LM75_ReadTemp(&al_temp) == LM75_OK) {
                lcd_show_temp(1, "T:", al_temp);
                
                // Dieu kien thoat bao dong: Nhiet do da giam <= Thyst (39.0 C) va chan OS nha muc 1
                if(gpio_read(GPIOA, 5) == 1 && al_temp <= 390) {
                    flag_os_alarm = 0;
                    lcd_clear();
                    set_cursor(0, 0); lcd_send_string("NHIET DO DA HA  ");
                    set_cursor(0, 1); lcd_send_string("Tro ve RUN...   ");
                    delay(800);
                    sys_state = STATE_RUNNING;
                }
            } else {
                set_cursor(0, 1);
                lcd_send_string("T: LOI CAM BIEN ");
            }
            delay(150);
            break;

        default:
            sys_state = STATE_OFF;
            break;
    }
}
