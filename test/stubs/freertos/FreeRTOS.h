/* Host stub for FreeRTOS.h: only what keypad.c needs. */
#ifndef STUB_FREERTOS_H
#define STUB_FREERTOS_H

#include <stdint.h>

typedef uint32_t TickType_t;
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))

#endif /* STUB_FREERTOS_H */
