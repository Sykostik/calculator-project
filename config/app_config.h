#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include "esp_timer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Wi-Fi & Gemini API Configuration */
#define WIFI_SSID       "ESP32Hotspot"
#define WIFI_PASS       "12345678"
#define GEMINI_API_KEY  "AQ.Ab8RN6IxR4qVb0DlhSX5cKjXWBVTx2ZRHCYs1cnAabyMyVK9uQ"

/* LCD (ILI9341) Pins & Dimensions */
#define LCD_PIN_MOSI    14
#define LCD_PIN_SCK     48
#define LCD_PIN_CS      47
#define LCD_PIN_DC      3
#define LCD_PIN_RST     2
#define LCD_W           320
#define LCD_H           240

/* Camera (OV3660) Pins */
#define PWDN_GPIO_NUM   -1
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM   15
#define SIOD_GPIO_NUM   4
#define SIOC_GPIO_NUM   5
#define Y9_GPIO_NUM     16
#define Y8_GPIO_NUM     17
#define Y7_GPIO_NUM     18
#define Y6_GPIO_NUM     12
#define Y5_GPIO_NUM     10
#define Y4_GPIO_NUM     8
#define Y3_GPIO_NUM     9
#define Y2_GPIO_NUM     11
#define VSYNC_GPIO_NUM  6
#define HREF_GPIO_NUM   7
#define PCLK_GPIO_NUM   13

/* UI Characters & Color Palette (RGB565) */
#define DEG_CHAR        ((char)127)

#define COL_BG          0x0841
#define COL_HEADER      0x1926
#define COL_DISP_BG     0x0000
#define COL_DISP_BDR    0x4A69
#define COL_KEY_BG      0x10A2
#define COL_KEY_BDR     0x3186
#define COL_ACCENT      0x07FF
#define COL_BADGE_DEG   0xFD20
#define COL_BADGE_RAD   0x07E0
#define COL_ANS         0x07E0
#define COL_WHITE       0xFFFF
#define COL_BLACK       0x0000
#define COL_RED         0xF800
#define COL_AXIS        0x6B4D
#define COL_TICK_TXT    0x9CF3

/* Buffer & Feature Limits */
#define MAX_EXPR_LEN    128
#define MAX_HTTP_RESP   16384
#define MAX_AI_LINES    160
#define LINES_PER_PAGE  10
#define MAX_LINE_CHARS  48
#define CAM_FRAME_MS    33
#define MAX_CHAT_TURNS  8
#define MAX_CHAT_TEXT   1024

static inline int64_t millis_now(void) {
    return esp_timer_get_time() / 1000;
}

#endif /* APP_CONFIG_H */
