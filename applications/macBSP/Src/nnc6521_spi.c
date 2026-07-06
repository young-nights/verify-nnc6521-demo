/**
  ******************************************************************************
  * @file    nnc6521_spi.c
  * @brief   NNC6521 软件 SPI（GPIO 位操作）实现
  *          CPOL=0, CPHA=0, MSB 先传。支持双芯片独立引脚映射。
  ******************************************************************************
  */

#include "nnc6521.h"

/* ============================================================================
 *  两颗芯片的引脚映射表
 *  芯片 1：MOSI=PA7, CSN=PA4, SCLK=PA5, MISO=PA6, CHIP_EN=PC5, INTB=PC4
 *  芯片 2：MOSI=PB15, CSN=PB12, SCLK=PB13, MISO=PB14, CHIP_EN=PC7, INTB=PC6
 * ===========================================================================*/
static const nnc6521_pin_map_t nnc6521_pins[NNC6521_NUM_CHIPS] = {
    /* 芯片 1 */
    {
        .mosi_port   = GPIOA, .mosi_pin   = GPIO_PIN_7,
        .csn_port    = GPIOA, .csn_pin    = GPIO_PIN_4,
        .sclk_port   = GPIOA, .sclk_pin   = GPIO_PIN_5,
        .miso_port   = GPIOA, .miso_pin   = GPIO_PIN_6,
        .chip_en_port= GPIOC, .chip_en_pin= GPIO_PIN_5,
        .intb_port   = GPIOC, .intb_pin   = GPIO_PIN_4,
    },
    /* 芯片 2 */
    {
        .mosi_port   = GPIOB, .mosi_pin   = GPIO_PIN_15,
        .csn_port    = GPIOB, .csn_pin    = GPIO_PIN_12,
        .sclk_port   = GPIOB, .sclk_pin   = GPIO_PIN_13,
        .miso_port   = GPIOB, .miso_pin   = GPIO_PIN_14,
        .chip_en_port= GPIOC, .chip_en_pin= GPIO_PIN_7,
        .intb_port   = GPIOC, .intb_pin   = GPIO_PIN_6,
    },
};

/* ============================================================================
 *  GPIO 初始化
 * ===========================================================================*/

/**
 * @brief 使能指定 GPIO 端口的时钟
 *
 * @param[in] port  GPIO 端口指针（GPIOA/GPIOB/GPIOC）
 */
static void nnc6521_enable_gpio_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA)      __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
}

/**
 * @brief 初始化两颗 NNC6521 芯片的所有 GPIO 引脚
 *
 * 对每颗芯片执行以下初始化：
 * - MOSI（主出从入）：推挽输出，高速模式，初始低电平
 * - CSN（片选）：推挽输出，高速模式，初始高电平（空闲状态）
 * - SCLK（时钟）：推挽输出，高速模式，初始低电平（CPOL=0）
 * - CHIP_EN（芯片使能）：推挽输出，高速模式，初始低电平（禁用状态）
 * - MISO（主入从出）：浮空输入，无上下拉
 * - INTB（中断输出）：浮空输入，无上下拉
 *
 * @note 必须在任何 SPI 操作之前调用
 * @note 函数内部会自动使能所有需要的 GPIO 端口时钟
 *
 * @code
 * nnc6521_gpio_init();  // 初始化所有引脚
 * @endcode
 *
 * @see nnc6521_init
 */
void nnc6521_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    for (uint8_t i = 0; i < NNC6521_NUM_CHIPS; i++)
    {
        const nnc6521_pin_map_t *p = &nnc6521_pins[i];

        /* 使能该芯片使用的所有端口时钟 */
        nnc6521_enable_gpio_clock(p->mosi_port);
        nnc6521_enable_gpio_clock(p->csn_port);
        nnc6521_enable_gpio_clock(p->sclk_port);
        nnc6521_enable_gpio_clock(p->miso_port);
        nnc6521_enable_gpio_clock(p->chip_en_port);
        nnc6521_enable_gpio_clock(p->intb_port);

        /* MOSI：推挽输出，高速 */
        gpio.Pin   = p->mosi_pin;
        gpio.Mode  = GPIO_MODE_OUTPUT_PP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(p->mosi_port, &gpio);

        /* CSN：推挽输出（空闲高电平） */
        gpio.Pin = p->csn_pin;
        HAL_GPIO_Init(p->csn_port, &gpio);
        HAL_GPIO_WritePin(p->csn_port, p->csn_pin, GPIO_PIN_SET);

        /* SCLK：推挽输出（空闲低电平，CPOL=0） */
        gpio.Pin = p->sclk_pin;
        HAL_GPIO_Init(p->sclk_port, &gpio);
        HAL_GPIO_WritePin(p->sclk_port, p->sclk_pin, GPIO_PIN_RESET);

        /* CHIP_EN：推挽输出（空闲低电平，芯片禁用） */
        gpio.Pin = p->chip_en_pin;
        HAL_GPIO_Init(p->chip_en_port, &gpio);
        HAL_GPIO_WritePin(p->chip_en_port, p->chip_en_pin, GPIO_PIN_RESET);

        /* MISO：浮空输入 */
        gpio.Pin  = p->miso_pin;
        gpio.Mode = GPIO_MODE_INPUT;
        gpio.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(p->miso_port, &gpio);

        /* INTB：浮空输入 */
        gpio.Pin = p->intb_pin;
        HAL_GPIO_Init(p->intb_port, &gpio);
    }

    /* Output enable signals (PB0/PB1/PB10/PB11, active high) */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    gpio.Pin   = GPIO_PIN_0 | GPIO_PIN_1;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_SET);

    gpio.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);

    gpio.Pin = GPIO_PIN_11;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
}

/* ============================================================================
 *  软件 SPI 核心传输（CPOL=0, CPHA=0, MSB 先传）
 * ===========================================================================*/

/**
 * @brief 软件 SPI 字节传输（全双工）
 *
 * 时序说明（CPOL=0, CPHA=0）：
 * - SCLK 空闲低电平
 * - 数据在 SCLK 上升沿采样，在下降沿切换
 * - MSB（bit 7）先传
 *
 * @param[in] chip_id  芯片编号
 * @param[in] tx_byte  要发送的字节
 *
 * @return 接收到的字节
 */
static uint8_t spi_sw_transfer_byte(uint8_t chip_id, uint8_t tx_byte)
{
    const nnc6521_pin_map_t *p = &nnc6521_pins[chip_id];
    uint8_t rx_byte = 0;

    for (int8_t bit = 7; bit >= 0; bit--)
    {
        /* MOSI setup: set data before clock edge */
        if (tx_byte & (1 << bit))
            HAL_GPIO_WritePin(p->mosi_port, p->mosi_pin, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(p->mosi_port, p->mosi_pin, GPIO_PIN_RESET);

        __NOP(); __NOP(); __NOP(); __NOP();  /* Data setup time (~55ns @ 72MHz) */

        /* SCLK rising edge: slave latches MOSI, master latches MISO */
        HAL_GPIO_WritePin(p->sclk_port, p->sclk_pin, GPIO_PIN_SET);

        __NOP(); __NOP(); __NOP(); __NOP();  /* Clock high time (~55ns) */
        __NOP(); __NOP(); __NOP(); __NOP();  /* Extra margin for NNC6521 */

        /* Read MISO */
        if (HAL_GPIO_ReadPin(p->miso_port, p->miso_pin) == GPIO_PIN_SET)
            rx_byte |= (1 << bit);

        /* SCLK falling edge */
        HAL_GPIO_WritePin(p->sclk_port, p->sclk_pin, GPIO_PIN_RESET);

        __NOP(); __NOP(); __NOP(); __NOP();  /* Clock low time */
    }

    return rx_byte;
}

/* ============================================================================
 *  NNC6521 SPI 协议层
 * ===========================================================================*/

/**
 * @brief 通过软件 SPI 向 NNC6521 寄存器写入一个字节
 *
 * 写入协议为 4 字节时序：[addr, cmd, 0x11, data]
 * - 字节 0：寄存器地址
 * - 字节 1：命令字（普通寄存器 cmd=0x80，波形寄存器 cmd=0xC0）
 * - 字节 2：数据标记（固定为 0x11）
 * - 字节 3：写入数据
 *
 * SPI 模式：CPOL=0, CPHA=0, MSB 先传。
 *
 * @param[in] chip_id  芯片编号，取值 NNC6521_CHIP_1 或 NNC6521_CHIP_2
 * @param[in] addr     寄存器地址
 * @param[in] data     要写入的数据字节
 * @param[in] is_wave  1=波形寄存器（cmd 0xC0），0=普通寄存器（cmd 0x80）
 *
 * @code
 * // 向芯片 1 的普通寄存器 0x01 写入 0x00
 * nnc6521_spi_write(NNC6521_CHIP_1, 0x01, 0x00, 0);
 *
 * // 向芯片 1 的波形寄存器 0x04 写入 0xFF
 * nnc6521_spi_write(NNC6521_CHIP_1, 0x04, 0xFF, 1);
 * @endcode
 *
 * @see nnc6521_spi_read, nnc6521_write_reg, nnc6521_write_wave_reg
 */
void nnc6521_spi_write(uint8_t chip_id, uint8_t addr, uint8_t data, uint8_t is_wave)
{
    const nnc6521_pin_map_t *p = &nnc6521_pins[chip_id];
    uint8_t cmd = is_wave ? 0xC0 : 0x80;

    HAL_GPIO_WritePin(p->csn_port, p->csn_pin, GPIO_PIN_RESET);
    __NOP(); __NOP(); __NOP(); __NOP();

    spi_sw_transfer_byte(chip_id, addr);
    spi_sw_transfer_byte(chip_id, cmd);
    spi_sw_transfer_byte(chip_id, data);   /* data in byte 2 */
    spi_sw_transfer_byte(chip_id, 0x00);   /* dummy byte 3 */

    __NOP(); __NOP(); __NOP(); __NOP();
    HAL_GPIO_WritePin(p->csn_port, p->csn_pin, GPIO_PIN_SET);
}

/**
 * @brief 通过软件 SPI 从 NNC6521 寄存器读取一个字节
 *
 * 读取协议为 3 字节时序：[addr, cmd, dummy]，数据在第 3 字节返回。
 * - 字节 0：寄存器地址
 * - 字节 1：命令字（普通寄存器 cmd=0x00，波形寄存器 cmd=0x40）
 * - 字节 2：空操作字节，MISO 上返回实际数据
 *
 * @param[in] chip_id  芯片编号，取值 NNC6521_CHIP_1 或 NNC6521_CHIP_2
 * @param[in] addr     寄存器地址
 * @param[in] is_wave  1=波形寄存器（cmd 0x40），0=普通寄存器（cmd 0x00）
 *
 * @return 读取到的数据字节
 *
 * @code
 * // 从芯片 1 的普通寄存器 0x01 读取
 * uint8_t val = nnc6521_spi_read(NNC6521_CHIP_1, 0x01, 0);
 * @endcode
 *
 * @see nnc6521_spi_write, nnc6521_read_reg, nnc6521_read_wave_reg
 */
uint8_t nnc6521_spi_read(uint8_t chip_id, uint8_t addr, uint8_t is_wave)
{
    const nnc6521_pin_map_t *p = &nnc6521_pins[chip_id];
    uint8_t cmd = is_wave ? 0x40 : 0x00;
    uint8_t rx[3];

    HAL_GPIO_WritePin(p->csn_port, p->csn_pin, GPIO_PIN_RESET);
    __NOP(); __NOP(); __NOP(); __NOP();

    rx[0] = spi_sw_transfer_byte(chip_id, addr);
    rx[1] = spi_sw_transfer_byte(chip_id, cmd);
    rx[2] = spi_sw_transfer_byte(chip_id, 0x00);

    __NOP(); __NOP(); __NOP(); __NOP();
    HAL_GPIO_WritePin(p->csn_port, p->csn_pin, GPIO_PIN_SET);

    return rx[2];
}

/**
 * @brief 从 OTP 存储器读取一个字节
 *
 * OTP（One-Time Programmable）读取流程：
 * 1. 向寄存器 0x1D 写入 OTP 地址（设置要读取的地址）
 * 2. 向寄存器 0x1B 写入 0x54（发送 OTP 读命令）
 * 3. 从寄存器 0x1E 读取数据（获取 OTP 数据）
 * 4. 向寄存器 0x1B 写入 0x50（结束 OTP 读取操作）
 *
 * @param[in] chip_id  芯片编号，取值 NNC6521_CHIP_1 或 NNC6521_CHIP_2
 * @param[in] addr     OTP 地址
 *
 * @return OTP 数据字节
 *
 * @note OTP 存储器为一次性可编程，只能读取不能写入
 * @note 校准数据通常存储在地址 0x00~0x63 范围内
 *
 * @code
 * // 读取芯片 1 的 OTP 地址 0x00 处的校准数据
 * uint8_t cal = nnc6521_spi_otp_read(NNC6521_CHIP_1, 0x00);
 * @endcode
 *
 * @see nnc6521_spi_read
 */
uint8_t nnc6521_spi_otp_read(uint8_t chip_id, uint8_t addr)
{
    uint8_t output;

    nnc6521_write_reg(chip_id, 0x1D, addr);     /* 设置 OTP 地址 */
    nnc6521_write_reg(chip_id, 0x1B, 0x54);     /* OTP 读命令 */
    output = nnc6521_read_reg(chip_id, 0x1E);   /* 读取 OTP 数据 */
    nnc6521_write_reg(chip_id, 0x1B, 0x50);     /* 结束 OTP 读取 */

    return output;
}
