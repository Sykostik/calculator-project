#include "keypad.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

static const int row_pins[4] = {45, 21, 1, 42};
static const int col_pins[4] = {41, 40, 39, 38};
static const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

void keypad_init(void) {
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(row_pins[i]);
        gpio_set_direction(row_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(row_pins[i], 1);

        gpio_reset_pin(col_pins[i]);
        gpio_set_direction(col_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(col_pins[i], GPIO_PULLUP_ONLY);
    }
}

char keypad_scan(void) {
    static char last_held = 0;
    char current = 0;

    for (int r = 0; r < 4; r++) {
        gpio_set_level(row_pins[r], 0);
        esp_rom_delay_us(40);
        for (int c = 0; c < 4; c++) {
            if (gpio_get_level(col_pins[c]) == 0) {
                current = keymap[r][c];
                break;
            }
        }
        gpio_set_level(row_pins[r], 1);
        if (current != 0) break;
    }

    if (current != 0 && current != last_held) {
        last_held = current;
        vTaskDelay(pdMS_TO_TICKS(15));
        return current;
    }
    if (current == 0) {
        last_held = 0;
    }
    return 0;
}
