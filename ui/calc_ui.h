#ifndef CALC_UI_H
#define CALC_UI_H

#include <stdbool.h>
#include <stdint.h>

void calc_draw_ui(void);
void calc_update_header(void);
void calc_draw_keymap_legend(void);
void calc_clear_result_area(void);
void calc_handle_key(char key);
void calc_tick(int64_t now_ms);
void calc_mark_dirty(void);

const char *calc_get_expression(void);
int calc_get_expr_len(void);
void calc_set_clipboard(const char *text);

#endif /* CALC_UI_H */
