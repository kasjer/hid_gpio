/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <assert.h>

#include "bsp/bsp.h"
#include "os/mynewt.h"

#include <hal/hal_bsp.h>
#include <hal/hal_flash_int.h>
#include <hal/hal_system.h>

#include <stm32_common/stm32_hal.h>

#if MYNEWT_VAL(PWM_0) || MYNEWT_VAL(PWM_1) || MYNEWT_VAL(PWM_2)
#include <pwm_stm32/pwm_stm32.h>
#endif

#if MYNEWT_VAL(ADC_0) || MYNEWT_VAL(ADC_1) || MYNEWT_VAL(ADC_2)
#include "stm32f4xx_hal_adc.h"
#include "adc_stm32f4/adc_stm32f4.h"
#endif

#if MYNEWT_VAL(SPIFLASH)
#include <spiflash/spiflash.h>
#endif

const uint32_t stm32_flash_sectors[] = {
    0x08000000,     /* 16kB */
    0x08004000,     /* 16kB */
    0x08008000,     /* 16kB */
    0x0800c000,     /* 16kB */
    0x08010000,     /* 64kB */
    0x08020000,     /* 128kB */
    0x08080000,     /* End of flash */
};

#define SZ ARRAY_SIZE(stm32_flash_sectors)
static_assert(MYNEWT_VAL(STM32_FLASH_NUM_AREAS) + 1 == SZ,
        "STM32_FLASH_NUM_AREAS does not match flash sectors");

#if MYNEWT_VAL(UART_0)
const struct stm32_uart_cfg os_bsp_uart0_cfg = {
    .suc_uart = USART2,
    .suc_rcc_reg = &RCC->APB1ENR,
    .suc_rcc_dev = RCC_APB1ENR_USART2EN,
    .suc_pin_tx = MYNEWT_VAL(UART_0_PIN_TX),
    .suc_pin_rx = MYNEWT_VAL(UART_0_PIN_RX),
    .suc_pin_rts = MYNEWT_VAL(UART_0_PIN_RTS),
    .suc_pin_cts = MYNEWT_VAL(UART_0_PIN_CTS),
    .suc_pin_af = GPIO_AF7_USART2,
    .suc_irqn = USART2_IRQn,
};
#endif

static const struct hal_bsp_mem_dump dump_cfg[] = {
    [0] = {
        .hbmd_start = &_ram_start,
        .hbmd_size = RAM_SIZE
    }
};

extern const struct hal_flash stm32_flash_dev;

#if MYNEWT_VAL(SPIFLASH)
#if MYNEWT_VAL(BUS_DRIVER_PRESENT)
struct bus_spi_node_cfg flash_spi_cfg = {
    .node_cfg.bus_name = MYNEWT_VAL(BSP_FLASH_SPI_BUS),
    .pin_cs = MYNEWT_VAL(SPIFLASH_SPI_CS_PIN),
    .mode = BUS_SPI_MODE_3,
    .data_order = HAL_SPI_MSB_FIRST,
    .freq = MYNEWT_VAL(SPIFLASH_BAUDRATE),
};
#endif
#endif

#if MYNEWT_VAL(I2C_0)
/*
 * NOTE: The PB8 and PB9 pins are connected through jumpers in the board to
 * both AIN and I2C pins. To enable I2C functionality SB51/SB56 need to
 * be removed (they are the default connections) and SB46/SB52 need to
 * be added.
 */
const struct stm32_hal_i2c_cfg os_bsp_i2c0_cfg = {
    .hic_i2c = I2C1,
    .hic_rcc_reg = &RCC->APB1ENR,
    .hic_rcc_dev = RCC_APB1ENR_I2C1EN,
    .get_pclk = STM32_I2C1_GET_CLOCK_FREQ,
    .hic_pin_sda = MYNEWT_VAL(I2C_0_PIN_SDA),
    .hic_pin_scl = MYNEWT_VAL(I2C_0_PIN_SCL),
    .hic_pin_af = GPIO_AF4_I2C1,
    .hic_10bit = 0,
    .hic_speed = 100000,                    /* 100kHz */
};
#endif

static const struct hal_flash *flash_devs[] = {
    [0] = &stm32_flash_dev,
#if MYNEWT_VAL(SPIFLASH)
    [1] = &spiflash_dev.hal,
#endif
};

const struct hal_flash *
hal_bsp_flash_dev(uint8_t id)
{
    if (id >= ARRAY_SIZE(flash_devs)) {
        return NULL;
    }

    return flash_devs[id];
}

const struct hal_bsp_mem_dump *
hal_bsp_core_dump(int *area_cnt)
{
    *area_cnt = ARRAY_SIZE(dump_cfg);
    return dump_cfg;
}

void
hal_bsp_init(void)
{
    stm32_periph_create();
}

void
hal_bsp_deinit(void)
{
    RCC->AHB1ENR = 0;
    RCC->AHB2ENR = 0;
    RCC->APB1ENR = 0;
    RCC->APB2ENR = 0;
    RCC->AHB1RSTR = 0x00C0109F;
    RCC->AHB2RSTR = 0x00000080;
    RCC->APB1RSTR = 0x10E2C80F;
    RCC->APB2RSTR = 0x00077931;
    RCC->AHB1RSTR = 0x00000000;
    RCC->AHB2RSTR = 0x00000000;
    RCC->APB1RSTR = 0x00000000;
    RCC->APB2RSTR = 0x00000000;
}

/**
 * Returns the configured priority for the given interrupt. If no priority
 * configured, return the priority passed in
 *
 * @param irq_num
 * @param pri
 *
 * @return uint32_t
 */
uint32_t
hal_bsp_get_nvic_priority(int irq_num, uint32_t pri)
{
    /* Add any interrupt priorities configured by the bsp here */
    return pri;
}
