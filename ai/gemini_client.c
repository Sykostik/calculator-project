#include "gemini_client.h"
#include "chat_history.h"
#include "app_config.h"
#include "wifi_manager.h"
#include "camera.h"
#include "lcd.h"
#include "calc_ui.h"
#include "ai_overlay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "mbedtls/base64.h"

static const char *GEMINI_MODELS[] = {
    "gemini-flash-lite-latest",
    "gemini-flash-latest"
};
#define NUM_MODELS 2

static char http_resp_buf[MAX_HTTP_RESP];
static int http_resp_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (http_resp_len + evt->data_len < MAX_HTTP_RESP - 1) {
            memcpy(http_resp_buf + http_resp_len, evt->data, evt->data_len);
            http_resp_len += evt->data_len;
            http_resp_buf[http_resp_len] = '\0';
        }
    }
    return ESP_OK;
}

static void send_gemini_with_fallback(const char *payload, const char *title, const char *user_msg_for_history) {
    int status = -1;
    esp_err_t err = ESP_FAIL;

    for (int m = 0; m < NUM_MODELS; m++) {
        char url[256];
        snprintf(url, sizeof(url),
                 "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent",
                 GEMINI_MODELS[m]);

        http_resp_len = 0;
        http_resp_buf[0] = '\0';

        esp_http_client_config_t config = {
            .url = url,
            .method = HTTP_METHOD_POST,
            .event_handler = http_event_handler,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .timeout_ms = 35000
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "x-goog-api-key", GEMINI_API_KEY);
        esp_http_client_set_post_field(client, payload, strlen(payload));

        err = esp_http_client_perform(client);
        status = esp_http_client_get_status_code(client);
        esp_http_client_cleanup(client);

        if (err == ESP_OK && status == 200) break;

        if ((status == 503 || status == 404) && m < NUM_MODELS - 1) {
            ai_overlay_show(title, "Server busy...\nTrying backup model...");
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            break;
        }
    }

    if (err == ESP_OK && status == 200) {
        cJSON *root = cJSON_Parse(http_resp_buf);
        if (root != NULL) {
            cJSON *candidates = cJSON_GetObjectItem(root, "candidates");
            cJSON *first = cJSON_GetArrayItem(candidates, 0);
            cJSON *content = cJSON_GetObjectItem(first, "content");
            cJSON *parts = cJSON_GetObjectItem(content, "parts");
            cJSON *part0 = cJSON_GetArrayItem(parts, 0);
            cJSON *text = cJSON_GetObjectItem(part0, "text");
            if (text != NULL && cJSON_IsString(text)) {
                if (user_msg_for_history != NULL) {
                    chat_add(false, user_msg_for_history);
                    chat_add(true, text->valuestring);
                }
                ai_overlay_show(title, text->valuestring);
            } else {
                ai_overlay_show("AI ERROR", "Empty reply from Gemini.");
            }
            cJSON_Delete(root);
        } else {
            ai_overlay_show("JSON ERROR", "Failed to parse response.");
        }
    } else {
        char err_msg[96];
        if (status == 400 || status == 403) snprintf(err_msg, sizeof(err_msg), "HTTP %d\nCheck API Key.", status);
        else if (status == 404) snprintf(err_msg, sizeof(err_msg), "HTTP 404\nModel not found.");
        else if (status == 429) snprintf(err_msg, sizeof(err_msg), "HTTP 429\nRate limit hit. Wait 1 min.");
        else if (status == 503) snprintf(err_msg, sizeof(err_msg), "HTTP 503\nServers busy. Try again.");
        else snprintf(err_msg, sizeof(err_msg), "HTTP %d\nRequest failed.", status);
        ai_overlay_show("AI ERROR", err_msg);
    }
}

static void ask_gemini_custom(const char *prompt, const char *title, const char *history_label, int max_tokens) {
    cJSON *root = cJSON_CreateObject();
    cJSON *contents = cJSON_CreateArray();

    int count = chat_get_count();
    for (int k = 0; k < count; k++) {
        const chat_msg_t *msg = chat_get_msg(k);
        if (msg == NULL) continue;
        cJSON *msg_obj = cJSON_CreateObject();
        cJSON *msg_parts = cJSON_CreateArray();
        cJSON *msg_part = cJSON_CreateObject();
        cJSON_AddStringToObject(msg_obj, "role", msg->is_model ? "model" : "user");
        cJSON_AddStringToObject(msg_part, "text", msg->text);
        cJSON_AddItemToArray(msg_parts, msg_part);
        cJSON_AddItemToObject(msg_obj, "parts", msg_parts);
        cJSON_AddItemToArray(contents, msg_obj);
    }

    cJSON *content_obj = cJSON_CreateObject();
    cJSON *parts = cJSON_CreateArray();
    cJSON *part_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(content_obj, "role", "user");
    cJSON_AddStringToObject(part_obj, "text", prompt);
    cJSON_AddItemToArray(parts, part_obj);
    cJSON_AddItemToObject(content_obj, "parts", parts);
    cJSON_AddItemToArray(contents, content_obj);
    cJSON_AddItemToObject(root, "contents", contents);

    cJSON *gen_cfg = cJSON_CreateObject();
    cJSON_AddNumberToObject(gen_cfg, "maxOutputTokens", max_tokens);
    cJSON_AddItemToObject(root, "generationConfig", gen_cfg);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (payload != NULL) {
        send_gemini_with_fallback(payload, title, history_label);
        free(payload);
    }
}

static void sanitize_expression(const char *expr, int len, char *out, size_t out_sz) {
    size_t j = 0;
    for (int i = 0; i < len && j < out_sz - 5; i++) {
        if (expr[i] == DEG_CHAR) {
            memcpy(out + j, " deg", 4);
            j += 4;
        } else {
            out[j++] = expr[i];
        }
    }
    out[j] = '\0';
}

void gemini_ask_expression(bool show_steps) {
    int expr_len = calc_get_expr_len();
    if (expr_len == 0) {
        calc_clear_result_area();
        lcd_draw_text(14, 96, "Enter input first!", COL_RED, COL_DISP_BG, 2);
        return;
    }

    if (!wifi_connect()) return;
    ai_overlay_show("GEMINI AI", "Thinking...");

    char clean_expr[MAX_EXPR_LEN * 2];
    sanitize_expression(calc_get_expression(), expr_len, clean_expr, sizeof(clean_expr));

    char prompt[512];
    if (show_steps) {
        snprintf(prompt, sizeof(prompt),
                 "Give a complete, detailed step-by-step explanation showing all work clearly in plain ASCII English. No markdown, no LaTeX, no asterisks: %s",
                 clean_expr);
    } else {
        snprintf(prompt, sizeof(prompt),
                 "Give the answer with a brief 1 to 2 sentence explanation in plain ASCII English. No markdown, no LaTeX, no asterisks: %s",
                 clean_expr);
    }

    ask_gemini_custom(prompt, show_steps ? "GEMINI STEPS" : "GEMINI ANSWER", clean_expr, show_steps ? 900 : 250);
}

void gemini_ask_analysis(void) {
    int expr_len = calc_get_expr_len();
    if (expr_len == 0) {
        calc_clear_result_area();
        lcd_draw_text(14, 96, "Enter f(x) first!", COL_RED, COL_DISP_BG, 2);
        return;
    }

    if (!wifi_connect()) return;
    ai_overlay_show("FUNC ANALYSIS", "Analyzing f(x)...");

    char clean_expr[MAX_EXPR_LEN * 2];
    sanitize_expression(calc_get_expression(), expr_len, clean_expr, sizeof(clean_expr));

    char prompt[640];
    snprintf(prompt, sizeof(prompt),
             "Perform a basic function analysis for f(x) = %s in plain ASCII English (no markdown, no LaTeX, no asterisks). List clearly: 1. Domain D(f) 2. Range E(f) 3. Parity (even/odd/neither) 4. Zeros & intercepts (Ox, Oy) 5. Sign intervals (f(x)>0, f(x)<0) 6. Monotonicity & extrema (inc/dec, min/max) 7. Asymptotes (vertical, horizontal, oblique).",
             clean_expr);

    char hist[192];
    snprintf(hist, sizeof(hist), "Analyze f(x) = %.150s", clean_expr);
    ask_gemini_custom(prompt, "FUNC ANALYSIS", hist, 900);
}

void gemini_capture_and_solve_photo(void) {
    camera_set_active(false);

    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL || fb->len < 100) {
        if (fb != NULL) esp_camera_fb_return(fb);
        ai_overlay_show("CAM ERROR", "Failed to capture photo.");
        return;
    }

    uint8_t *jpg_buf = NULL;
    size_t jpg_len = 0;

    if (!frame2jpg(fb, 90, &jpg_buf, &jpg_len)) {
        esp_camera_fb_return(fb);
        ai_overlay_show("CAM ERROR", "JPEG encode failed.");
        return;
    }

    esp_camera_fb_return(fb);

    ai_overlay_show("GEMINI CAM", "Photo snapped!\nConnecting & sending...");

    if (!wifi_connect()) {
        free(jpg_buf);
        calc_draw_ui();
        calc_mark_dirty();
        return;
    }

    ai_overlay_show("GEMINI CAM", "Analyzing photo...");

    size_t b64_len = 0;
    mbedtls_base64_encode(NULL, 0, &b64_len, jpg_buf, jpg_len);
    char *b64_buf = (char *)heap_caps_malloc(b64_len + 1, MALLOC_CAP_SPIRAM);
    if (b64_buf == NULL) b64_buf = (char *)malloc(b64_len + 1);
    if (b64_buf == NULL) {
        free(jpg_buf);
        ai_overlay_show("AI ERROR", "Out of memory for photo.");
        return;
    }

    size_t out_len = 0;
    if (mbedtls_base64_encode((unsigned char *)b64_buf, b64_len + 1, &out_len, jpg_buf, jpg_len) != 0) {
        free(b64_buf);
        free(jpg_buf);
        ai_overlay_show("AI ERROR", "Base64 encode failed.");
        return;
    }
    b64_buf[out_len] = '\0';
    free(jpg_buf);

    chat_clear();

    const char *prompt = "Look at this photo and answer or explain what is shown clearly in plain ASCII English. If there is a question or problem, solve it with steps. No markdown, no LaTeX, no asterisks.";
    size_t payload_size = out_len + 512;
    char *payload = (char *)heap_caps_malloc(payload_size, MALLOC_CAP_SPIRAM);
    if (payload == NULL) payload = (char *)malloc(payload_size);
    if (payload != NULL) {
        snprintf(payload, payload_size,
                 "{\"contents\":[{\"role\":\"user\",\"parts\":[{\"text\":\"%s\"},{\"inline_data\":{\"mime_type\":\"image/jpeg\",\"data\":\"%s\"}}]}],\"generationConfig\":{\"maxOutputTokens\":700}}",
                 prompt, b64_buf);
        send_gemini_with_fallback(payload, "GEMINI CAM", "[User showed a photo]");
        free(payload);
    }
    free(b64_buf);
}
