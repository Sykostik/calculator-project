#ifndef CHAT_HISTORY_H
#define CHAT_HISTORY_H

#include <stdbool.h>
#include "app_config.h"

typedef struct {
    bool is_model;
    char text[MAX_CHAT_TEXT];
} chat_msg_t;

void chat_init(void);
void chat_clear(void);
void chat_add(bool is_model, const char *text);
int chat_get_count(void);
const chat_msg_t *chat_get_msg(int index);

#endif /* CHAT_HISTORY_H */
