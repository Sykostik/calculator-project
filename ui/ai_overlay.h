#ifndef AI_OVERLAY_H
#define AI_OVERLAY_H

#include <stdbool.h>

bool ai_overlay_is_active(void);
void ai_overlay_show(const char *title, const char *body);
void ai_overlay_handle_key(char key);

#endif /* AI_OVERLAY_H */
