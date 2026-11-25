#ifndef CAMERA_H
#define CAMERA_H

#include <stdbool.h>
#include <stdint.h>

void camera_start_mode(void);
bool camera_is_active(void);
void camera_set_active(bool active);
void camera_render_preview(void);
void camera_tick(int64_t now_ms);

#endif /* CAMERA_H */
