/*
 * Host stub for ESP-IDF driver/gpio.h.
 * It simulates the 4x4 keypad matrix: the test "presses" a key with
 * sim_keypad_press(row, col), and gpio_get_level() on a column returns 0
 * only while that key's row pin is driven low — exactly like the real wiring.
 */
#ifndef STUB_DRIVER_GPIO_H
#define STUB_DRIVER_GPIO_H

typedef int gpio_num_t;

typedef enum {
    GPIO_MODE_INPUT = 1,
    GPIO_MODE_OUTPUT = 2
} gpio_mode_t;

typedef enum {
    GPIO_PULLUP_ONLY = 0
} gpio_pull_mode_t;

void gpio_reset_pin(int pin);
void gpio_set_direction(int pin, gpio_mode_t mode);
void gpio_set_pull_mode(int pin, gpio_pull_mode_t mode);
void gpio_set_level(int pin, int level);
int gpio_get_level(int pin);

/* Test helpers (implemented in test/host_stubs.c). */
void sim_keypad_press(int row, int col);
void sim_keypad_release(void);

#endif /* STUB_DRIVER_GPIO_H */
