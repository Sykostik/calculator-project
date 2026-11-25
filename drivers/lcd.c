#include "lcd.h"
#include "font5x7.h"
#include "app_config.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static spi_device_handle_t lcd_spi;
static uint16_t line_buf[LCD_W];
static uint16_t char_buf[18 * 21];

static uint16_t swap16(uint16_t c) {
    return (uint16_t)((c >> 8) | (c << 8));
}

static void lcd_cmd(uint8_t cmd) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    gpio_set_level(LCD_PIN_DC, 0);
    spi_device_polling_transmit(lcd_spi, &t);
}

static void lcd_data(const uint8_t *data, int len) {
    if (len <= 0) return;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    gpio_set_level(LCD_PIN_DC, 1);
    spi_device_polling_transmit(lcd_spi, &t);
}

static void lcd_data_byte(uint8_t val) {
    lcd_data(&val, 1);
}

static void lcd_set_window(int x0, int y0, int x1, int y1) {
    uint8_t data[4];
    lcd_cmd(0x2A);
    data[0] = (x0 >> 8) & 0xFF;
    data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF;
    data[3] = x1 & 0xFF;
    lcd_data(data, 4);

    lcd_cmd(0x2B);
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;
    lcd_data(data, 4);

    lcd_cmd(0x2C);
}

void lcd_fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LCD_W) w = LCD_W - x;
    if (y + h > LCD_H) h = LCD_H - y;
    if (w <= 0 || h <= 0) return;

    uint16_t swapped = swap16(color);
    for (int i = 0; i < w; i++) {
        line_buf[i] = swapped;
    }

    lcd_set_window(x, y, x + w - 1, y + h - 1);
    gpio_set_level(LCD_PIN_DC, 1);
    for (int row = 0; row < h; row++) {
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = w * 16;
        t.tx_buffer = line_buf;
        spi_device_polling_transmit(lcd_spi, &t);
    }
}

void lcd_draw_rect(int x, int y, int w, int h, uint16_t color) {
    lcd_fill_rect(x, y, w, 1, color);
    lcd_fill_rect(x, y + h - 1, w, 1, color);
    lcd_fill_rect(x, y, 1, h, color);
    lcd_fill_rect(x + w - 1, y, 1, h, color);
}

void lcd_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, int scale) {
    unsigned char uc = (unsigned char)c;
    if (uc < 32 || uc > 127) uc = '?';
    if (scale < 1) scale = 1;
    if (scale > 3) scale = 3;

    const uint8_t *glyph = font5x7[uc - 32];
    uint16_t fg_s = swap16(fg);
    uint16_t bg_s = swap16(bg);
    int cw = 6 * scale;
    int ch = 7 * scale;

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 6; col++) {
            uint16_t pix = bg_s;
            if (col < 5 && (glyph[col] & (1 << row))) {
                pix = fg_s;
            }
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    char_buf[(row * scale + sy) * cw + (col * scale + sx)] = pix;
                }
            }
        }
    }

    lcd_set_window(x, y, x + cw - 1, y + ch - 1);
    gpio_set_level(LCD_PIN_DC, 1);
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = cw * ch * 16;
    t.tx_buffer = char_buf;
    spi_device_polling_transmit(lcd_spi, &t);
}

void lcd_draw_text(int x, int y, const char *str, uint16_t fg, uint16_t bg, int scale) {
    int cx = x;
    int i = 0;
    while (str[i] != '\0') {
        if (cx + 6 * scale > LCD_W - 2) break;
        lcd_draw_char(cx, y, str[i], fg, bg, scale);
        cx += 6 * scale;
        i++;
    }
}

void lcd_draw_rgb565_frame(int x, int y, int draw_w, int draw_h, int src_stride, const uint16_t *src) {
    if (draw_w <= 0 || draw_h <= 0 || draw_w > LCD_W) return;

    lcd_set_window(x, y, x + draw_w - 1, y + draw_h - 1);
    gpio_set_level(LCD_PIN_DC, 1);
    for (int row = 0; row < draw_h; row++) {
        memcpy(line_buf, src + row * src_stride, draw_w * sizeof(uint16_t));
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = draw_w * 16;
        t.tx_buffer = line_buf;
        spi_device_polling_transmit(lcd_spi, &t);
    }
}

void lcd_init(void) {
    gpio_reset_pin(LCD_PIN_DC);
    gpio_set_direction(LCD_PIN_DC, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LCD_PIN_RST);
    gpio_set_direction(LCD_PIN_RST, GPIO_MODE_OUTPUT);

    spi_bus_config_t buscfg = {0};
    buscfg.mosi_io_num = LCD_PIN_MOSI;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = LCD_PIN_SCK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = LCD_W * 2 * 16;
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {0};
    devcfg.clock_speed_hz = 40 * 1000 * 1000;
    devcfg.mode = 0;
    devcfg.spics_io_num = LCD_PIN_CS;
    devcfg.queue_size = 7;
    spi_bus_add_device(SPI2_HOST, &devcfg, &lcd_spi);

    gpio_set_level(LCD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd_cmd(0x28);
    lcd_cmd(0x3A);
    lcd_data_byte(0x55);
    lcd_cmd(0x36);
    lcd_data_byte(0x28);
    lcd_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(20));
}
