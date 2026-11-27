/* Host stub for esp_rom_sys.h: microsecond delay does nothing on the PC. */
#ifndef STUB_ESP_ROM_SYS_H
#define STUB_ESP_ROM_SYS_H

#include <stdint.h>

static inline void esp_rom_delay_us(uint32_t us) {
    (void)us;
}

#endif /* STUB_ESP_ROM_SYS_H */
