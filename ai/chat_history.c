#include "chat_history.h"

#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"

static chat_msg_t *chat_history = NULL;
static int chat_count = 0;

void chat_init(void) {
    if (chat_history == NULL) {
        chat_history = (chat_msg_t *)heap_caps_calloc(MAX_CHAT_TURNS, sizeof(chat_msg_t), MALLOC_CAP_SPIRAM);
        if (chat_history == NULL) {
            chat_history = (chat_msg_t *)calloc(MAX_CHAT_TURNS, sizeof(chat_msg_t));
        }
    }
    chat_count = 0;
}

void chat_clear(void) {
    chat_count = 0;
}

void chat_add(bool is_model, const char *text) {
    if (chat_history == NULL || text == NULL) return;
    if (chat_count >= MAX_CHAT_TURNS) {
        memmove(&chat_history[0], &chat_history[2], (MAX_CHAT_TURNS - 2) * sizeof(chat_msg_t));
        chat_count -= 2;
    }
    chat_history[chat_count].is_model = is_model;
    strncpy(chat_history[chat_count].text, text, MAX_CHAT_TEXT - 1);
    chat_history[chat_count].text[MAX_CHAT_TEXT - 1] = '\0';
    chat_count++;
}

int chat_get_count(void) {
    return chat_count;
}

const chat_msg_t *chat_get_msg(int index) {
    if (chat_history == NULL || index < 0 || index >= chat_count) return NULL;
    return &chat_history[index];
}
