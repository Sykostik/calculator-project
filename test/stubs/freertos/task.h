/* Host stub for FreeRTOS task.h: a delay just does nothing on the PC. */
#ifndef STUB_FREERTOS_TASK_H
#define STUB_FREERTOS_TASK_H

#include "freertos/FreeRTOS.h"

static inline void vTaskDelay(TickType_t ticks) {
    (void)ticks;
}

#endif /* STUB_FREERTOS_TASK_H */
