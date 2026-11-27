/* Host stub for ESP-IDF esp_heap_caps.h: PSRAM allocations become normal heap allocations. */
#ifndef STUB_ESP_HEAP_CAPS_H
#define STUB_ESP_HEAP_CAPS_H

#include <stdlib.h>

#define MALLOC_CAP_SPIRAM (1 << 10)

static inline void *heap_caps_calloc(size_t n, size_t size, unsigned caps) {
    (void)caps;
    return calloc(n, size);
}

static inline void *heap_caps_malloc(size_t size, unsigned caps) {
    (void)caps;
    return malloc(size);
}

#endif /* STUB_ESP_HEAP_CAPS_H */
