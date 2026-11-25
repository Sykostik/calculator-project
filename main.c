#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "app_config.h"
#include "lcd.h"
#include "keypad.h"
#include "camera.h"
#include "chat_history.h"
#include "gemini_client.h"
#include "calc_ui.h"
#include "graph_ui.h"
#include "ai_overlay.h"

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    chat_init();
    lcd_init();
    keypad_init();
    calc_draw_ui();

    while (1) {
        char key = keypad_scan();
        if (key != 0) {
            if (graph_is_active()) {
                graph_handle_key(key);
            } else if (camera_is_active()) {
                if (key == '#' || key == '0') {
                    gemini_capture_and_solve_photo();
                } else {
                    camera_set_active(false);
                    calc_draw_ui();
                    calc_mark_dirty();
                }
            } else if (ai_overlay_is_active()) {
                ai_overlay_handle_key(key);
            } else {
                calc_handle_key(key);
                calc_mark_dirty();
            }
        }

        int64_t now = millis_now();

        if (camera_is_active() && !ai_overlay_is_active() && !graph_is_active()) {
            camera_tick(now);
        }

        if (!ai_overlay_is_active() && !camera_is_active() && !graph_is_active()) {
            calc_tick(now);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
