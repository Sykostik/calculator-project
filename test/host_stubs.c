/*
 * host_stubs.c — replacements for hardware and network functions,
 * so the calculator logic can be compiled and tested on a normal PC.
 *
 *  - LCD:      instead of drawing, every text is saved to a log,
 *              so tests can check what would appear on the screen.
 *  - GPIO:     a simulated 4x4 keypad matrix.
 *  - Wi-Fi, camera, Gemini, graph mode: empty stubs that only count calls
 *              (they need real hardware/internet and are tested on the device).
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "host_stubs.h"
#include "driver/gpio.h"

/* ---------- fake clock (used by millis_now) ---------- */
int64_t g_fake_time_us = 0;

/* ---------- LCD: text log ---------- */
#define LOG_SIZE 32
#define LOG_TEXT 128
static char lcd_log[LOG_SIZE][LOG_TEXT];
static int lcd_log_count = 0;

void stub_lcd_clear_log(void) {
    lcd_log_count = 0;
}

const char *stub_lcd_find(const char *prefix) {
    for (int i = lcd_log_count - 1; i >= 0; i--) {
        if (strncmp(lcd_log[i % LOG_SIZE], prefix, strlen(prefix)) == 0) {
            return lcd_log[i % LOG_SIZE];
        }
    }
    return NULL;
}

void lcd_init(void) {}
void lcd_fill_rect(int x, int y, int w, int h, uint16_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
}
void lcd_draw_rect(int x, int y, int w, int h, uint16_t color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
}
void lcd_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, int scale) {
    (void)x; (void)y; (void)c; (void)fg; (void)bg; (void)scale;
}
void lcd_draw_text(int x, int y, const char *str, uint16_t fg, uint16_t bg, int scale) {
    (void)x; (void)y; (void)fg; (void)bg; (void)scale;
    strncpy(lcd_log[lcd_log_count % LOG_SIZE], str, LOG_TEXT - 1);
    lcd_log[lcd_log_count % LOG_SIZE][LOG_TEXT - 1] = '\0';
    lcd_log_count++;
}
void lcd_draw_rgb565_frame(int x, int y, int draw_w, int draw_h, int src_stride, const uint16_t *src) {
    (void)x; (void)y; (void)draw_w; (void)draw_h; (void)src_stride; (void)src;
}

/* ---------- GPIO: simulated keypad matrix ---------- */
static int pin_level[64];
static int pressed_row_pin = -1;
static int pressed_col_pin = -1;
static const int sim_rows[4] = {45, 21, 1, 42};
static const int sim_cols[4] = {41, 40, 39, 38};

void gpio_reset_pin(int pin) { if (pin >= 0 && pin < 64) pin_level[pin] = 1; }
void gpio_set_direction(int pin, gpio_mode_t mode) { (void)pin; (void)mode; }
void gpio_set_pull_mode(int pin, gpio_pull_mode_t mode) { (void)pin; (void)mode; }
void gpio_set_level(int pin, int level) { if (pin >= 0 && pin < 64) pin_level[pin] = level; }

int gpio_get_level(int pin) {
    /* A column reads 0 only if its key is pressed AND that key's row is driven low. */
    if (pin == pressed_col_pin && pressed_row_pin >= 0 && pin_level[pressed_row_pin] == 0) {
        return 0;
    }
    return 1; /* pull-up */
}

void sim_keypad_press(int row, int col) {
    pressed_row_pin = sim_rows[row];
    pressed_col_pin = sim_cols[col];
}

void sim_keypad_release(void) {
    pressed_row_pin = -1;
    pressed_col_pin = -1;
}

/* ---------- Wi-Fi, camera, Gemini, graph: counted stubs ---------- */
int stub_calls_gemini = 0;
int stub_calls_camera = 0;
int stub_calls_graph = 0;
int stub_calls_wifi = 0;

bool wifi_is_connected(void) { return false; }
bool wifi_connect(void) { stub_calls_wifi++; return false; }
void wifi_disconnect(void) {}

void camera_start_mode(void) { stub_calls_camera++; }

void gemini_ask_expression(bool show_steps) { (void)show_steps; stub_calls_gemini++; }
void gemini_ask_analysis(void) { stub_calls_gemini++; }

void graph_start_mode(void) { stub_calls_graph++; }
