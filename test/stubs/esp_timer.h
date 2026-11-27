/*
 * Host stub for ESP-IDF esp_timer.h.
 * The tests control "time" through g_fake_time_us, so timing logic
 * (for example the 800 ms T9 timeout) can be checked without real hardware.
 */
#ifndef STUB_ESP_TIMER_H
#define STUB_ESP_TIMER_H

#include <stdint.h>

extern int64_t g_fake_time_us;

static inline int64_t esp_timer_get_time(void) {
    return g_fake_time_us;
}

#endif /* STUB_ESP_TIMER_H */
