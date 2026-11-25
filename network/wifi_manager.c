#include "wifi_manager.h"
#include "app_config.h"
#include "lcd.h"
#include "calc_ui.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"

#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t wifi_event_group;
static bool wifi_connected = false;
static bool wifi_started = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_connected = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_once(void) {
    if (wifi_started) return;
    wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    wifi_started = true;
}

bool wifi_is_connected(void) {
    return wifi_connected;
}

bool wifi_connect(void) {
    if (wifi_connected) return true;

    calc_clear_result_area();
    lcd_draw_text(14, 96, "Connecting WiFi...", COL_ACCENT, COL_DISP_BG, 2);

    wifi_init_once();
    esp_wifi_start();
    esp_wifi_set_max_tx_power(60);
    esp_wifi_connect();

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(15000));
    calc_update_header();
    calc_clear_result_area();

    if ((bits & WIFI_CONNECTED_BIT) != 0) {
        lcd_draw_text(14, 96, "WiFi Connected!", COL_ANS, COL_DISP_BG, 2);
        return true;
    } else {
        esp_wifi_disconnect();
        lcd_draw_text(14, 96, "WiFi Failed!", COL_RED, COL_DISP_BG, 2);
        return false;
    }
}

void wifi_disconnect(void) {
    if (wifi_started) {
        esp_wifi_disconnect();
        esp_wifi_stop();
    }
    wifi_connected = false;
    calc_update_header();
    calc_clear_result_area();
    lcd_draw_text(14, 96, "WiFi Disconnected", COL_BADGE_DEG, COL_DISP_BG, 2);
}
