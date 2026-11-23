/**
 * @file    i2c_driver.c
 * @brief   Trinh dieu khien giao tiep I2C1 Bare-metal cap thanh ghi cho STM32F103
 * @details Trien khai cac chuc nang Master theo dung ST RM0008:
 *          - Tan so xung nhip: Standard Mode 100 kHz (CCR=180, TRISE=37, APB1=36MHz)
 *          - Chuyen PB6, PB7 sang che do Alternate Function Open-Drain
 *          - Thuat toan nhan 1 byte va 2 byte theo RM0008 Section 26.3.3
 *          - Thuat toan cuu ho bus bi ket (9 xung SCL bit-bang theo NXP UM10204)
 */

#include "stm32f10x.h"
#include "stdint.h"
#include "i2c_driver.h"
#include "gpio_driver.h"

#define I2C_TIMEOUT_LOOPS   20000UL
#define I2C_CHECK(expr)     do { i2c_status_t _s = (expr); if (_s != I2C_OK) return _s; } while (0)

/**
 * @brief Cau hinh phan cung khoi ngoai vi I2C1
 */
static void i2c_hw_init(void)
{
    // 1. Reset I2C1 bang phan mem de lam sach cac co trang thai cu
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    // 2. Thiet lap tan so bus APB1 = 36 MHz vao thanh ghi CR2
    I2C1->CR2   = 36;

    // 3. Cau hinh dinh thoi 100 kHz (Standard Mode):
    //    T_high = 5000ns, T_PCLK1 = 1 / 36MHz = 27.78ns -> CCR = 5000 / 27.78 = 180
    I2C1->CCR   = 180;

    // 4. Cau hinh thoi gian suon tang toi da (1000ns theo chuan Standard Mode):
    //    TRISE = (1000ns / 27.78ns) + 1 = 37
    I2C1->TRISE = 37;

    // 5. Bat cho phep ngoai vi I2C1
    I2C1->CR1  |= I2C_CR1_PE;
}

/**
 * @brief Cho mot hoac nhieu co trang thai tren thanh ghi SR1, co kiem tra timeout va loi bus
 */
static i2c_status_t i2c_wait(uint32_t ok_mask)
{
    uint32_t timeout = I2C_TIMEOUT_LOOPS;
    uint32_t sr1;

    while (1)
    {
        sr1 = I2C1->SR1;
        if (sr1 & ok_mask)                          return I2C_OK;
        if (sr1 & I2C_SR1_AF)                       return I2C_ERR_NACK;
        if (sr1 & (I2C_SR1_BERR | I2C_SR1_ARLO))    return I2C_ERR_BUS;
        if (--timeout == 0)                         return I2C_ERR_TIMEOUT;
    }
}

/**
 * @brief Xu ly khi gap loi I2C, dua bus ve trang thai an toan
 */
static i2c_status_t i2c_fail(i2c_status_t status)
{
    if (status == I2C_ERR_NACK) {
        I2C1->SR1 &= ~I2C_SR1_AF;   // Xoa co AF
        I2C1->CR1 |= I2C_CR1_STOP;  // Phat dieu kien STOP de giai phong bus
    } else {
        i2c_recover();              // Loi bus hoac timeout: chay chuoi cuu ho bus
    }
    return status;
}

/**
 * @brief Khoi tao giao tiep I2C1 cap GPIO va cap thanh ghi
 */
void i2c_init(void)
{
    // 1. Bat clock ngoai vi I2C1, GPIOB va AFIO
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    gpio_enable_clock(GPIOB);
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    // 2. Cau hinh chan PB6 (SCL) va PB7 (SDA) che do Alternate Function Open-Drain
    gpio_init(GPIOB, 6, GPIO_MODE_OUTPUT_50M, GPIO_CNF_AF_OD);
    gpio_init(GPIOB, 7, GPIO_MODE_OUTPUT_50M, GPIO_CNF_AF_OD);

    // 3. Nap thiet lap thanh ghi I2C1
    i2c_hw_init();
}

/**
 * @brief Giai phong bus khi thiet bi to bi treo keo chan SDA xuong muc thap
 *        Thuc hien bit-bang 9 xung clock SCL theo dac ta NXP UM10204 Section 3.1.16.
 */
void i2c_recover(void)
{
    // 1. Tat ngoai vi I2C1
    I2C1->CR1 &= ~I2C_CR1_PE;

    // 2. Chuyen PB6 va PB7 sang GPIO Open-Drain thong thuong de tao xung thu cong
    gpio_init(GPIOB, 6, GPIO_MODE_OUTPUT_50M, GPIO_CNF_GP_OD);
    gpio_init(GPIOB, 7, GPIO_MODE_OUTPUT_50M, GPIO_CNF_GP_OD);
    GPIOB->BSRR = (1U << 6) | (1U << 7);

    for (volatile int i = 0; i < 400; i++) __NOP();

    // 3. Phat toi da 9 xung clock tren SCL de buoc Slave nha duong SDA
    for (uint8_t i = 0; i < 9; i++) {
        if (GPIOB->IDR & (1U << 7)) break; // Slave da nha SDA len muc cao
        GPIOB->BRR  = (1U << 6); for (volatile int j = 0; j < 400; j++) __NOP();
        GPIOB->BSRR = (1U << 6); for (volatile int j = 0; j < 400; j++) __NOP();
    }

    // 4. Phat dieu kien STOP thu cong tren bus
    GPIOB->BRR  = (1U << 7); for (volatile int j = 0; j < 400; j++) __NOP();
    GPIOB->BSRR = (1U << 6); for (volatile int j = 0; j < 400; j++) __NOP();
    GPIOB->BSRR = (1U << 7); for (volatile int j = 0; j < 400; j++) __NOP();

    // 5. Tra lai chuc nang ngoai vi I2C (AF Open-Drain) va nap lai cau hinh
    gpio_init(GPIOB, 6, GPIO_MODE_OUTPUT_50M, GPIO_CNF_AF_OD);
    gpio_init(GPIOB, 7, GPIO_MODE_OUTPUT_50M, GPIO_CNF_AF_OD);
    i2c_hw_init();
}

/**
 * @brief Cho cho den khi bus ranh (Co BUSY = 0 tren I2C1->SR2)
 */
i2c_status_t i2c_wait_bus_free(void)
{
    uint32_t timeout = I2C_TIMEOUT_LOOPS;
    while (I2C1->SR2 & I2C_SR2_BUSY) {
        if (--timeout == 0) {
            i2c_recover();
            return (I2C1->SR2 & I2C_SR2_BUSY) ? I2C_ERR_BUS : I2C_OK;
        }
    }
    return I2C_OK;
}

/**
 * @brief Phat tin hieu START tren bus I2C
 */
i2c_status_t i2c_start(uint8_t mode)
{
    if (mode == I2C_MODE_MASTER) {
        I2C1->CR1 |= I2C_CR1_START;
        i2c_status_t st = i2c_wait(I2C_SR1_SB);
        if (st != I2C_OK) return i2c_fail(st);
    }
    return I2C_OK;
}

/**
 * @brief Phat tin hieu STOP tren bus I2C
 */
void i2c_stop(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    uint32_t to = 1000;
    while ((I2C1->CR1 & I2C_CR1_STOP) && --to);
}

/**
 * @brief Gui dia chi 7-bit cua thiet bi to kem theo bit R/W
 */
i2c_status_t i2c_send_address(uint8_t address, uint8_t rw)
{
    I2C1->DR = (uint8_t)((address << 1) | (rw & 0x1));
    i2c_status_t st = i2c_wait(I2C_SR1_ADDR);
    if (st != I2C_OK) return i2c_fail(st);

    // Xoa co ADDR bang cach doc SR1 sau do doc SR2 theo dung quy trinh cua RM0008
    volatile uint32_t temp = I2C1->SR1;
    temp = I2C1->SR2;
    (void)temp;

    return I2C_OK;
}

/**
 * @brief Gui 1 byte du lieu tren bus I2C
 */
i2c_status_t i2c_write_data(uint8_t data)
{
    i2c_status_t st = i2c_wait(I2C_SR1_TXE);
    if (st != I2C_OK) return i2c_fail(st);

    I2C1->DR = data;

    st = i2c_wait(I2C_SR1_BTF);
    if (st != I2C_OK) return i2c_fail(st);

    return I2C_OK;
}

/**
 * @brief Ghi 1 byte du lieu vao thiet bi to I2C
 */
i2c_status_t i2c_write_byte(uint8_t slave_address, uint8_t data)
{
    I2C_CHECK(i2c_wait_bus_free());
    I2C_CHECK(i2c_start(I2C_MODE_MASTER));
    I2C_CHECK(i2c_send_address(slave_address, 0));
    I2C_CHECK(i2c_write_data(data));
    i2c_stop();
    return I2C_OK;
}

/**
 * @brief Ghi mot mang byte du lieu vao thiet bi to I2C
 */
i2c_status_t i2c_write_mul_bytes(uint8_t slave_address, uint8_t *data, uint8_t length)
{
    I2C_CHECK(i2c_wait_bus_free());
    I2C_CHECK(i2c_start(I2C_MODE_MASTER));
    I2C_CHECK(i2c_send_address(slave_address, 0));

    for (uint8_t i = 0; i < length; i++) {
        I2C_CHECK(i2c_write_data(data[i]));
    }

    i2c_stop();
    return I2C_OK;
}

/**
 * @brief Giai doan doc 1 hoac 2 byte theo dung chuan ST RM0008 Section 26.3.3
 */
static i2c_status_t i2c_read_phase(uint8_t slave_address, uint8_t *data, uint8_t length)
{
    volatile uint32_t tmp;
    i2c_status_t st;

    if (length == 2) {
        // Doc 2 byte: Thiet lap bit POS va ACK truoc khi gui tin hieu START
        I2C1->CR1 |= (I2C_CR1_ACK | I2C_CR1_POS);
    } else {
        // Doc 1 byte: Thiet lap bit ACK
        I2C1->CR1 |= I2C_CR1_ACK;
    }

    I2C_CHECK(i2c_start(I2C_MODE_MASTER));
    I2C1->DR = (uint8_t)((slave_address << 1) | 1);

    st = i2c_wait(I2C_SR1_ADDR);
    if (st != I2C_OK) return i2c_fail(st);

    if (length == 1) {
        // Nhan 1 byte theo RM0008:
        // Tat ACK truoc khi xoa ADDR -> xoa ADDR -> phat STOP -> cho RXNE -> doc DR
        I2C1->CR1 &= ~I2C_CR1_ACK;
        tmp = I2C1->SR1; tmp = I2C1->SR2;
        I2C1->CR1 |= I2C_CR1_STOP;

        st = i2c_wait(I2C_SR1_RXNE);
        if (st != I2C_OK) return i2c_fail(st);

        data[0] = (uint8_t)I2C1->DR;
    } else {
        // Nhan 2 byte theo RM0008 (dung bit POS):
        // Xoa ADDR -> tat ACK -> cho BTF (ca 2 byte da nhan) -> phat STOP -> doc 2 byte tu DR
        tmp = I2C1->SR1; tmp = I2C1->SR2;
        I2C1->CR1 &= ~I2C_CR1_ACK;

        st = i2c_wait(I2C_SR1_BTF);
        if (st != I2C_OK) return i2c_fail(st);

        I2C1->CR1 |= I2C_CR1_STOP;
        data[0] = (uint8_t)I2C1->DR;
        data[1] = (uint8_t)I2C1->DR;
    }

    (void)tmp;
    return I2C_OK;
}

/**
 * @brief Khoi phuc thanh ghi CR1 ve trang thai mac dinh sau khi doc
 */
static void i2c_read_restore(void)
{
    I2C1->CR1 &= ~I2C_CR1_POS;
    I2C1->CR1 |= I2C_CR1_ACK;
}

/**
 * @brief Doc 1 hoac 2 byte tu thiet bi to qua bus I2C
 */
i2c_status_t i2c_read_mul_bytes(uint8_t slave_address, uint8_t *data, uint8_t length)
{
    i2c_status_t st;
    if (length != 1 && length != 2) return I2C_ERR_BUS;
    st = i2c_wait_bus_free();
    if (st == I2C_OK) st = i2c_read_phase(slave_address, data, length);
    i2c_read_restore();
    return st;
}

/**
 * @brief Doc chuoi nhieu byte lien tuc (Sequential Read)
 */
i2c_status_t i2c_read_buffer(uint8_t slave_address, uint8_t *data, uint8_t length)
{
    i2c_status_t st;
    if (data == 0 || length == 0) return I2C_ERR_BUS;

    st = i2c_wait_bus_free();
    if (st != I2C_OK) return st;

    I2C1->CR1 |= I2C_CR1_ACK;
    I2C_CHECK(i2c_start(I2C_MODE_MASTER));
    I2C1->DR = (uint8_t)((slave_address << 1) | 1);
    st = i2c_wait(I2C_SR1_ADDR);
    if (st != I2C_OK) return i2c_fail(st);

    volatile uint32_t tmp = I2C1->SR1;
    tmp = I2C1->SR2;
    (void)tmp;

    if (length == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        st = i2c_wait(I2C_SR1_RXNE);
        if (st != I2C_OK) return i2c_fail(st);
        I2C1->CR1 |= I2C_CR1_STOP;
        data[0] = (uint8_t)I2C1->DR;
    } else {
        for (uint8_t i = 0; i < length - 1; i++) {
            st = i2c_wait(I2C_SR1_RXNE);
            if (st != I2C_OK) return i2c_fail(st);
            data[i] = (uint8_t)I2C1->DR;
        }
        // Byte cuoi cung: tat ACK, phat STOP roi doc byte cuoi tu DR
        I2C1->CR1 &= ~I2C_CR1_ACK;
        st = i2c_wait(I2C_SR1_RXNE);
        if (st != I2C_OK) return i2c_fail(st);
        I2C1->CR1 |= I2C_CR1_STOP;
        data[length - 1] = (uint8_t)I2C1->DR;
    }

    i2c_read_restore();
    return I2C_OK;
}

/**
 * @brief Ghi wlen byte dia chi roi phat Repeated START doc rlen byte du lieu (Random Read)
 */
i2c_status_t i2c_write_read(uint8_t slave_address, const uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen)
{
    i2c_status_t st;
    if (rlen != 1 && rlen != 2) return I2C_ERR_BUS;

    st = i2c_wait_bus_free();
    if (st == I2C_OK) st = i2c_start(I2C_MODE_MASTER);
    if (st == I2C_OK) st = i2c_send_address(slave_address, 0);

    for (uint8_t i = 0; st == I2C_OK && i < wlen; i++) {
        st = i2c_write_data(wdata[i]);
    }

    if (st == I2C_OK) st = i2c_read_phase(slave_address, rdata, rlen);

    i2c_read_restore();
    return st;
}
