// SPDX-License-Identifier: MIT

#include "hal.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/dwt.h>
#include <string.h>
#include <stdio.h>

// UART Configuration (matches CW308T-STM32F3)
#define SERIAL_BAUD 38400
#define SERIAL_GPIO GPIOA
#define SERIAL_USART USART1
#define SERIAL_PINS (GPIO9 | GPIO10)

static void clock_setup(void)
{
    // Enable HSE oscillator
    rcc_periph_clock_enable(RCC_GPIOH);
    rcc_osc_off(RCC_HSE);
    rcc_osc_bypass_enable(RCC_HSE);
    rcc_osc_on(RCC_HSE);
    rcc_wait_for_osc_ready(RCC_HSE);

    // Configure Flash latency for 36 MHz
    flash_set_ws(FLASH_ACR_LATENCY(1));

    // Turn off PLL before configuration
    rcc_osc_off(RCC_PLL);

    // Configure PLL: HSE × 5 = 36 MHz
    RCC_CFGR &= ~(RCC_CFGR_PLLSRC | (RCC_CFGR_PLLMUL_MASK << RCC_CFGR_PLLMUL_SHIFT));
    RCC_CFGR |= (RCC_CFGR_PLLSRC_HSE_PREDIV << 16) |
                (RCC_CFGR_PLLMUL_MUL5 << RCC_CFGR_PLLMUL_SHIFT);

    // Enable PLL
    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);

    // Set bus dividers (no division)
    rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE);
    rcc_set_ppre1(RCC_CFGR_PPRE1_DIV_NONE);
    rcc_set_ppre2(RCC_CFGR_PPRE2_DIV_NONE);

    // Switch system clock to PLL
    rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
    rcc_wait_for_sysclk_status(RCC_PLL);

    // Update frequency variables
    rcc_ahb_frequency = 36000000;
    rcc_apb1_frequency = 36000000;
    rcc_apb2_frequency = 36000000;
}

// Initialize USART1 for serial communication
static void usart_setup(void)
{
    // Enable clocks for GPIOA and USART1
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);

    // CHANGE THESE THREE LINES to match PQM4:
    gpio_set_output_options(SERIAL_GPIO, GPIO_OTYPE_OD, GPIO_OSPEED_100MHZ, SERIAL_PINS);
    gpio_set_af(SERIAL_GPIO, GPIO_AF7, SERIAL_PINS);
    gpio_mode_setup(SERIAL_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, SERIAL_PINS);

    // Configure USART parameters
    usart_set_baudrate(SERIAL_USART, SERIAL_BAUD);
    usart_set_databits(SERIAL_USART, 8);
    usart_set_stopbits(SERIAL_USART, USART_STOPBITS_1);
    usart_set_mode(SERIAL_USART, USART_MODE_TX_RX);
    usart_set_parity(SERIAL_USART, USART_PARITY_NONE);
    usart_set_flow_control(SERIAL_USART, USART_FLOWCONTROL_NONE);
    
    // ADD THESE (from PQM4):
    usart_disable_rx_interrupt(SERIAL_USART);
    usart_disable_tx_interrupt(SERIAL_USART);

    // Enable USART
    usart_enable(SERIAL_USART);
}

static void dwt_setup(void) {
    // Enable DWT cycle counter for precise timing
    dwt_enable_cycle_counter();
}

static volatile unsigned long long overflowcnt = 0;

// Systick interrupt handler
void sys_tick_handler(void)
{
    ++overflowcnt;
}

static void systick_setup(void) {
    // Systick is always the same on libopencm3
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(0xFFFFFFu);
    systick_interrupt_enable();  // CRITICAL!
    systick_counter_enable();
}

void hal_setup(void) {
    clock_setup();
    usart_setup();
    dwt_setup();
    systick_setup();  // ADD THIS
    
    // CRITICAL: Wait for the first systick overflow
    // This synchronizes with the host and prevents buffer overrun
    unsigned long long old = overflowcnt;
    while(old == overflowcnt);
}

// Simple delay loop to prevent serial buffer overrun
static void uart_delay(void) {
    // Wait ~5ms at 36MHz to let host process data
    for (volatile int i = 0; i < 180000; i++);
}

// Remove the static keyword to match the declaration in hal.h
void hal_send_str(const char *str)
{
    while (*str) {
        usart_send_blocking(SERIAL_USART, *str++);
    }
    // Send newline so host can detect end of line
    usart_send_blocking(SERIAL_USART, '\n');
    // Small delay to prevent SAM3U buffer overrun
    uart_delay();
}

void hal_send_number(unsigned long long num) {
    char buffer[32];
    sprintf(buffer, "%llu", num);
    
    for (char *p = buffer; *p; p++) {
        usart_send_blocking(SERIAL_USART, *p);
    }
    usart_send_blocking(SERIAL_USART, '\n');
    // Small delay to prevent SAM3U buffer overrun
    uart_delay();
}

uint64_t hal_get_time(void) {
    return dwt_read_cycle_counter();
}