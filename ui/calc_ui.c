#include "calc_ui.h"
#include "app_config.h"
#include "lcd.h"
#include "wifi_manager.h"
#include "camera.h"
#include "chat_history.h"
#include "gemini_client.h"
#include "math_parser.h"
#include "graph_ui.h"

#include <stdio.h>
#include <string.h>

static char expression[MAX_EXPR_LEN] = "";
static char clipboard[MAX_EXPR_LEN] = "";
static int expr_len = 0;
static int cursor_pos = 0;
static int64_t last_cursor_blink = 0;
static bool cursor_state = true;
static bool screen_dirty = true;

static char last_letter_key = 0;
static int last_letter_idx = 0;
static int64_t last_letter_time = 0;

static int current_layer = 0;

const char *calc_get_expression(void) {
    return expression;
}

int calc_get_expr_len(void) {
    return expr_len;
}

void calc_set_clipboard(const char *text) {
    if (text == NULL) {
        clipboard[0] = '\0';
        return;
    }
    strncpy(clipboard, text, MAX_EXPR_LEN - 1);
    clipboard[MAX_EXPR_LEN - 1] = '\0';
}

void calc_mark_dirty(void) {
    screen_dirty = true;
}

void calc_update_header(void) {
    const char *layer_names[6] = {"Numbers", "Layer A", "Layer B", "Layer C", "Layer D", "Letters"};
    lcd_fill_rect(0, 0, LCD_W, 28, COL_HEADER);
    lcd_fill_rect(0, 28, LCD_W, 1, COL_ACCENT);

    lcd_draw_text(8, 6, "CALC", COL_WHITE, COL_HEADER, 2);

    lcd_fill_rect(68, 4, 116, 20, COL_KEY_BG);
    lcd_draw_rect(68, 4, 116, 20, COL_ACCENT);
    char mode_str[24];
    snprintf(mode_str, sizeof(mode_str), "MODE: %s", layer_names[current_layer]);
    lcd_draw_text(74, 10, mode_str, COL_ACCENT, COL_KEY_BG, 1);

    uint16_t wifi_col = wifi_is_connected() ? COL_ANS : COL_DISP_BDR;
    lcd_fill_rect(232, 4, 24, 20, wifi_col);
    lcd_draw_text(241, 10, "W", COL_BLACK, wifi_col, 1);

    bool deg = math_is_deg_mode();
    uint16_t badge_col = deg ? COL_BADGE_DEG : COL_BADGE_RAD;
    lcd_fill_rect(262, 4, 50, 20, badge_col);
    lcd_draw_text(278, 10, deg ? "DEG" : "RAD", COL_BLACK, badge_col, 1);
}

void calc_draw_keymap_legend(void) {
    lcd_fill_rect(0, 132, LCD_W, 108, COL_BG);

    const char *num_labels[10]   = {"1: 1", "2: 2", "3: 3", "4: 4", "5: 5", "6: 6", "7: 7", "8: 8", "9: 9", "0: 0"};
    const char *lay_a_labels[10] = {"1: +", "2: -", "3: *", "4: /", "5: %", "6: .", "7: ()", "8:sqrt", "9: ^", "0:=SOLV"};
    const char *lay_b_labels[10] = {"1:sin", "2:cos", "3:tan", "4:ctg", "5:asin", "6:acos", "7:atan", "8:actg", "9:D/R", "0: deg"};
    const char *lay_c_labels[10] = {"1:log", "2:ln", "3: e", "4:pi", "5: | |", "6:fact", "7:cbrt", "8:exp", "9:Ans", "0:=SOLV"};
    const char *lay_d_labels[10] = {"1:Solv", "2:Step", "3:CAM", "4:WiFi", "5: x", "6:Grph", "7:d/dx", "8:Anlz", "9:NewC", "0:Copy"};
    const char *lay_l_labels[10] = {"1:abc", "2:def", "3:ghi", "4:jkl", "5:mno", "6:pqr", "7:stu", "8:vwx", "9:yz", "0:spc"};

    const char **active = num_labels;
    if (current_layer == 1) active = lay_a_labels;
    else if (current_layer == 2) active = lay_b_labels;
    else if (current_layer == 3) active = lay_c_labels;
    else if (current_layer == 4) active = lay_d_labels;
    else if (current_layer == 5) active = lay_l_labels;

    int card_w = 58;
    int card_h = 26;
    int start_x = 6;
    int start_y = 134;
    int gap_x = 4;
    int gap_y = 5;

    for (int i = 0; i < 10; i++) {
        int col = i % 5;
        int row = i / 5;
        int x = start_x + col * (card_w + gap_x);
        int y = start_y + row * (card_h + gap_y);

        bool is_solve = (i == 9 && (current_layer == 1 || current_layer == 3)) || (current_layer == 4 && i == 5);
        bool is_ai = (current_layer == 4 && (i == 0 || i == 1 || i == 2 || i == 7 || i == 8));
        bool is_clip = (current_layer == 4 && i == 9 && clipboard[0] != '\0');
        uint16_t bdr_col = is_solve ? COL_ANS : (is_clip ? COL_BADGE_DEG : (is_ai ? COL_ACCENT : COL_KEY_BDR));
        uint16_t txt_col = is_solve ? COL_ANS : (is_clip ? COL_BADGE_DEG : (is_ai ? COL_ACCENT : COL_WHITE));

        lcd_fill_rect(x, y, card_w, card_h, COL_KEY_BG);
        lcd_draw_rect(x, y, card_w, card_h, bdr_col);

        if (current_layer == 2 && i == 9) {
            char deg_lbl[8] = {'0', ':', ' ', DEG_CHAR, '\0'};
            lcd_draw_text(x + 5, y + 9, deg_lbl, txt_col, COL_KEY_BG, 1);
        } else if (current_layer == 4 && i == 9) {
            lcd_draw_text(x + 5, y + 9, clipboard[0] != '\0' ? "0:Pst" : "0:Copy", txt_col, COL_KEY_BG, 1);
        } else {
            lcd_draw_text(x + 5, y + 9, active[i], txt_col, COL_KEY_BG, 1);
        }
    }

    lcd_fill_rect(0, 218, LCD_W, 22, COL_HEADER);
    lcd_draw_text(8, 225, "[A] Layer  [B] 0-9/Abc  [C] Clr  [D] Del  [*][#]", 0xBDF7, COL_HEADER, 1);
}

void calc_draw_ui(void) {
    lcd_fill_rect(0, 0, LCD_W, LCD_H, COL_BG);
    lcd_fill_rect(6, 34, 308, 92, COL_DISP_BG);
    lcd_draw_rect(6, 34, 308, 92, COL_DISP_BDR);
    lcd_fill_rect(12, 82, 296, 1, 0x2104);

    calc_update_header();
    calc_draw_keymap_legend();
}

void calc_clear_result_area(void) {
    lcd_fill_rect(10, 85, 300, 38, COL_DISP_BG);
}

static void update_screen(void) {
    lcd_fill_rect(10, 38, 300, 42, COL_DISP_BG);

    if (expr_len == 0) {
        if (cursor_state) {
            lcd_draw_char(14, 48, '_', COL_ACCENT, COL_DISP_BG, 3);
        }
        return;
    }

    int scale = 3;
    int cy = 48;
    if (expr_len > 23) {
        scale = 1;
        cy = 40;
    } else if (expr_len > 14) {
        scale = 2;
        cy = 52;
    }

    int cw = 6 * scale;
    int cx = 14;

    for (int i = 0; i <= expr_len; i++) {
        if (i == cursor_pos && cursor_state) {
            if (cx + cw > 308) {
                if (scale == 1) {
                    cx = 14;
                    cy += 10;
                } else {
                    break;
                }
            }
            if (cy + 7 * scale > 80) break;
            lcd_draw_char(cx, cy, '|', COL_ACCENT, COL_DISP_BG, scale);
            cx += cw;
        }
        if (i < expr_len) {
            if (cx + cw > 308) {
                if (scale == 1) {
                    cx = 14;
                    cy += 10;
                } else {
                    break;
                }
            }
            if (cy + 7 * scale > 80) break;
            lcd_draw_char(cx, cy, expression[i], COL_WHITE, COL_DISP_BG, scale);
            cx += cw;
        }
    }
}

static void insert_str(const char *s) {
    int slen = strlen(s);
    if (expr_len + slen >= MAX_EXPR_LEN - 1) return;
    memmove(expression + cursor_pos + slen, expression + cursor_pos, expr_len - cursor_pos + 1);
    memcpy(expression + cursor_pos, s, slen);
    expr_len += slen;
    cursor_pos += slen;
}

static void insert_wrapped(const char *s) {
    int slen = strlen(s);
    if (expr_len + slen >= MAX_EXPR_LEN - 1) return;
    memmove(expression + cursor_pos + slen, expression + cursor_pos, expr_len - cursor_pos + 1);
    memcpy(expression + cursor_pos, s, slen);
    expr_len += slen;
    cursor_pos += (slen - 1);
}

void calc_handle_key(char key) {
    if (key == 'A') {
        last_letter_key = 0;
        if (current_layer == 0 || current_layer == 4 || current_layer == 5) current_layer = 1;
        else current_layer++;
        calc_update_header();
        calc_draw_keymap_legend();
        return;
    }
    if (key == 'B') {
        last_letter_key = 0;
        if (current_layer == 0) current_layer = 5;
        else current_layer = 0;
        calc_update_header();
        calc_draw_keymap_legend();
        return;
    }
    if (key == 'C') {
        last_letter_key = 0;
        if (expr_len == 0 && clipboard[0] != '\0') {
            clipboard[0] = '\0';
            calc_clear_result_area();
            lcd_draw_text(14, 96, "Clipboard Cleared", COL_BADGE_DEG, COL_DISP_BG, 2);
            if (current_layer == 4) calc_draw_keymap_legend();
            return;
        }
        expr_len = 0;
        cursor_pos = 0;
        expression[0] = '\0';
        calc_clear_result_area();
        return;
    }
    if (key == 'D') {
        last_letter_key = 0;
        if (cursor_pos > 0 && expr_len > 0) {
            memmove(expression + cursor_pos - 1, expression + cursor_pos, expr_len - cursor_pos + 1);
            expr_len--;
            cursor_pos--;
        }
        return;
    }
    if (key == '*') {
        last_letter_key = 0;
        if (cursor_pos > 0) cursor_pos--;
        return;
    }
    if (key == '#') {
        last_letter_key = 0;
        if (cursor_pos < expr_len) cursor_pos++;
        return;
    }

    if (current_layer == 0) {
        if (key >= '0' && key <= '9') {
            char tmp[2] = {key, '\0'};
            insert_str(tmp);
        }
    } else if (current_layer == 1) {
        if (key == '1') insert_str("+");
        else if (key == '2') insert_str("-");
        else if (key == '3') insert_str("*");
        else if (key == '4') insert_str("/");
        else if (key == '5') insert_str("%");
        else if (key == '6') insert_str(".");
        else if (key == '7') insert_wrapped("()");
        else if (key == '8') insert_wrapped("sqrt()");
        else if (key == '9') insert_str("^");
        else if (key == '0') math_evaluate_expression(expression, expr_len);
    } else if (current_layer == 2) {
        if (key == '1') insert_wrapped("sin()");
        else if (key == '2') insert_wrapped("cos()");
        else if (key == '3') insert_wrapped("tan()");
        else if (key == '4') insert_wrapped("ctg()");
        else if (key == '5') insert_wrapped("asin()");
        else if (key == '6') insert_wrapped("acos()");
        else if (key == '7') insert_wrapped("atan()");
        else if (key == '8') insert_wrapped("actg()");
        else if (key == '9') {
            math_toggle_deg_mode();
            calc_update_header();
        } else if (key == '0') {
            char tmp[2] = {DEG_CHAR, '\0'};
            insert_str(tmp);
        }
    } else if (current_layer == 3) {
        if (key == '1') insert_wrapped("log()");
        else if (key == '2') insert_wrapped("ln()");
        else if (key == '3') insert_str("e");
        else if (key == '4') insert_str("pi");
        else if (key == '5') insert_wrapped("||");
        else if (key == '6') insert_wrapped("fact()");
        else if (key == '7') insert_wrapped("cbrt()");
        else if (key == '8') insert_wrapped("exp()");
        else if (key == '9') insert_str("Ans");
        else if (key == '0') math_evaluate_expression(expression, expr_len);
    } else if (current_layer == 4) {
        if (key == '1') gemini_ask_expression(false);
        else if (key == '2') gemini_ask_expression(true);
        else if (key == '3') camera_start_mode();
        else if (key == '4') {
            if (wifi_is_connected()) wifi_disconnect();
            else wifi_connect();
        } else if (key == '5') insert_str("x");
        else if (key == '6') graph_start_mode();
        else if (key == '7') insert_wrapped("d/dx()");
        else if (key == '8') gemini_ask_analysis();
        else if (key == '9') {
            chat_clear();
            calc_clear_result_area();
            lcd_draw_text(14, 96, "New AI Chat Ready", COL_ACCENT, COL_DISP_BG, 2);
        } else if (key == '0') {
            if (clipboard[0] == '\0') {
                if (cursor_pos > 0) {
                    memcpy(clipboard, expression, cursor_pos);
                    clipboard[cursor_pos] = '\0';
                    calc_clear_result_area();
                    lcd_draw_text(14, 96, "Copied!", COL_ANS, COL_DISP_BG, 2);
                    calc_draw_keymap_legend();
                } else {
                    calc_clear_result_area();
                    lcd_draw_text(14, 96, "Nothing behind |", COL_RED, COL_DISP_BG, 2);
                }
            } else {
                insert_str(clipboard);
                calc_clear_result_area();
                lcd_draw_text(14, 96, "Pasted!", COL_ACCENT, COL_DISP_BG, 2);
            }
        }
    } else if (current_layer == 5) {
        if (key >= '1' && key <= '9') {
            const char *groups[9] = {"abc", "def", "ghi", "jkl", "mno", "pqr", "stu", "vwx", "yz"};
            const char *g = groups[key - '1'];
            int glen = strlen(g);
            int64_t now = millis_now();
            if (key == last_letter_key && (now - last_letter_time) < 800 && cursor_pos > 0) {
                last_letter_idx = (last_letter_idx + 1) % glen;
                expression[cursor_pos - 1] = g[last_letter_idx];
                last_letter_time = now;
            } else {
                last_letter_key = key;
                last_letter_idx = 0;
                char tmp[2] = {g[0], '\0'};
                insert_str(tmp);
                last_letter_time = now;
            }
        } else if (key == '0') {
            last_letter_key = 0;
            insert_str(" ");
        }
    }
}

void calc_tick(int64_t now_ms) {
    if (now_ms - last_cursor_blink > 500) {
        last_cursor_blink = now_ms;
        cursor_state = !cursor_state;
        screen_dirty = true;
    }

    if (screen_dirty) {
        update_screen();
        screen_dirty = false;
    }
}
