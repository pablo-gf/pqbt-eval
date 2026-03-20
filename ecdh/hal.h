#ifndef HAL_H
#define HAL_H

#include <stdint.h>

// Initialize hardware (clocks, UART, timers)
void hal_setup();

// Send string via UART
void hal_send_str(const char* str);

// Send number via UART
void hal_send_number(unsigned long long num);

// Get cycle counter for timing
uint64_t hal_get_time(void);

#endif // HAL_H