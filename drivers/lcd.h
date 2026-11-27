#ifndef DISPLAY_LCD_H
#define DISPLAY_LCD_H

#include <stdint.h>

void lcd_init(void);
void lcd_fill_rect(int x, int y, int w, int h, uint16_t color);
void lcd_draw_rect(int x, int y, int w, int h, uint16_t color);
void lcd_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, int scale);
void lcd_draw_text(int x, int y, const char *str, uint16_t fg, uint16_t bg, int scale);
void lcd_draw_rgb565_frame(int x, int y, int draw_w, int draw_h, int src_stride, const uint16_t *src);

#endif /* DRIVERS_LCD_H */
