/* Helpers that the tests use to look inside the stubbed hardware. */
#ifndef HOST_STUBS_H
#define HOST_STUBS_H

#include <stdint.h>

extern int64_t g_fake_time_us;

extern int stub_calls_gemini;
extern int stub_calls_camera;
extern int stub_calls_graph;
extern int stub_calls_wifi;

/* Forget all text "drawn" so far. */
void stub_lcd_clear_log(void);

/* The most recent text drawn on the LCD that starts with prefix, or NULL. */
const char *stub_lcd_find(const char *prefix);

#endif /* HOST_STUBS_H */
