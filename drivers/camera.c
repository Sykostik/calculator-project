#include "camera.h"
#include "app_config.h"
#include "lcd.h"
#include "calc_ui.h"

#include "esp_camera.h"

static bool camera_initialized = false;
static bool camera_active = false;
static int64_t last_cam_frame = 0;

static void apply_ov3660_tuning(sensor_t *s) {
    if (s == NULL) return;
    s->set_vflip(s, 1);
    s->set_hmirror(s, 0);
}

static bool init_camera_hw(void) {
    if (camera_initialized) return true;

    camera_config_t config = {0};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size   = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_RGB565;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count     = 2;

    if (esp_camera_init(&config) != ESP_OK) {
        return false;
    }

    apply_ov3660_tuning(esp_camera_sensor_get());
    camera_initialized = true;
    return true;
}

static void draw_viewfinder_frame(void) {
    lcd_fill_rect(0, 0, LCD_W, 28, COL_HEADER);
    lcd_fill_rect(0, 28, LCD_W, 1, COL_ACCENT);
    lcd_draw_text(10, 6, "CAM VIEWFINDER", COL_ACCENT, COL_HEADER, 2);

    lcd_fill_rect(0, 218, LCD_W, 22, COL_HEADER);
    lcd_draw_text(16, 225, "[#] or [0] Snap & Ask     [C] Exit", COL_ANS, COL_HEADER, 1);
}

void camera_start_mode(void) {
    calc_clear_result_area();
    lcd_draw_text(14, 96, "Starting CAM...", COL_ACCENT, COL_DISP_BG, 2);

    if (!init_camera_hw()) {
        calc_clear_result_area();
        lcd_draw_text(14, 96, "CAM Init Failed!", COL_RED, COL_DISP_BG, 2);
        return;
    }

    camera_active = true;
    last_cam_frame = 0;
    lcd_fill_rect(0, 0, LCD_W, LCD_H, COL_BG);
    draw_viewfinder_frame();
}

bool camera_is_active(void) {
    return camera_active;
}

void camera_set_active(bool active) {
    camera_active = active;
}

void camera_render_preview(void) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL) return;

    int draw_w = (int)fb->width < LCD_W ? (int)fb->width : LCD_W;
    int draw_h = (int)fb->height < 188 ? (int)fb->height : 188;
    int offset_x = (LCD_W - draw_w) / 2;
    int offset_y = 30 + (188 - draw_h) / 2;

    if (fb->format == PIXFORMAT_RGB565) {
        lcd_draw_rgb565_frame(offset_x, offset_y, draw_w, draw_h, (int)fb->width, (const uint16_t *)fb->buf);
    }

    esp_camera_fb_return(fb);
}

void camera_tick(int64_t now_ms) {
    if (camera_active && (now_ms - last_cam_frame >= CAM_FRAME_MS)) {
        last_cam_frame = now_ms;
        camera_render_preview();
    }
}
