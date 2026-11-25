#include "ai_overlay.h"
#include "app_config.h"
#include "lcd.h"
#include "calc_ui.h"

#include <stdio.h>
#include <string.h>

static bool ai_overlay_active = false;
static char ai_title[32] = "";
static char ai_lines[MAX_AI_LINES][MAX_LINE_CHARS + 1];
static int ai_line_count = 0;
static int ai_current_page = 0;
static int ai_total_pages = 1;
static int ai_cur_line = 0;
static int ai_cur_col = 0;
static int ai_sel_state = 0;
static int ai_mark_line = 0;
static int ai_mark_col = 0;
static int ai_end_line = 0;
static int ai_end_col = 0;
static bool ai_just_copied = false;

bool ai_overlay_is_active(void) {
    return ai_overlay_active;
}

static void paginate_ai_text(const char *text) {
    ai_line_count = 0;
    int len = strlen(text);
    int i = 0;
    char cur[MAX_LINE_CHARS + 1];
    int cur_len = 0;
    cur[0] = '\0';

    while (i < len && ai_line_count < MAX_AI_LINES) {
        if (text[i] == '\r') {
            i++;
            continue;
        }
        if (text[i] == '\n') {
            strcpy(ai_lines[ai_line_count++], cur);
            cur_len = 0;
            cur[0] = '\0';
            i++;
            continue;
        }

        int w_start = i;
        while (i < len && text[i] != ' ' && text[i] != '\n' && text[i] != '\r') {
            i++;
        }
        int w_len = i - w_start;
        if (i < len && text[i] == ' ') i++;

        if (w_len == 0) continue;

        if (cur_len > 0 && cur_len + 1 + w_len <= MAX_LINE_CHARS) {
            cur[cur_len++] = ' ';
            memcpy(cur + cur_len, text + w_start, w_len);
            cur_len += w_len;
            cur[cur_len] = '\0';
        } else if (cur_len == 0 && w_len <= MAX_LINE_CHARS) {
            memcpy(cur, text + w_start, w_len);
            cur_len = w_len;
            cur[cur_len] = '\0';
        } else {
            if (cur_len > 0) {
                strcpy(ai_lines[ai_line_count++], cur);
                cur_len = 0;
                cur[0] = '\0';
                if (ai_line_count >= MAX_AI_LINES) break;
            }
            int pos = 0;
            while (pos < w_len && ai_line_count < MAX_AI_LINES) {
                int chunk = w_len - pos;
                if (chunk > MAX_LINE_CHARS) chunk = MAX_LINE_CHARS;
                memcpy(cur, text + w_start + pos, chunk);
                cur[chunk] = '\0';
                cur_len = chunk;
                pos += chunk;
                if (pos < w_len) {
                    strcpy(ai_lines[ai_line_count++], cur);
                    cur_len = 0;
                    cur[0] = '\0';
                }
            }
        }
    }

    if (cur_len > 0 && ai_line_count < MAX_AI_LINES) {
        strcpy(ai_lines[ai_line_count++], cur);
    }
    if (ai_line_count == 0) {
        ai_lines[0][0] = '\0';
        ai_line_count = 1;
    }

    ai_current_page = 0;
    ai_cur_line = 0;
    ai_cur_col = 0;
    ai_sel_state = 0;
    ai_just_copied = false;
    ai_total_pages = (ai_line_count + LINES_PER_PAGE - 1) / LINES_PER_PAGE;
}

static void get_ai_sel_bounds(int *sl, int *sc, int *el, int *ec) {
    int l1 = ai_mark_line, c1 = ai_mark_col;
    int l2 = (ai_sel_state == 2) ? ai_end_line : ai_cur_line;
    int c2 = (ai_sel_state == 2) ? ai_end_col : ai_cur_col;
    if (l1 > l2 || (l1 == l2 && c1 > c2)) {
        *sl = l2; *sc = c2;
        *el = l1; *ec = c1;
    } else {
        *sl = l1; *sc = c1;
        *el = l2; *ec = c2;
    }
}

static void render_ai_page(void) {
    lcd_fill_rect(0, 0, LCD_W, LCD_H, COL_BG);

    lcd_fill_rect(0, 0, LCD_W, 28, COL_HEADER);
    lcd_fill_rect(0, 28, LCD_W, 1, COL_ACCENT);
    lcd_draw_text(10, 6, ai_title, COL_ACCENT, COL_HEADER, 2);

    if (ai_total_pages > 1) {
        char page_str[32];
        snprintf(page_str, sizeof(page_str), "%d/%d", ai_current_page + 1, ai_total_pages);
        lcd_fill_rect(256, 4, 56, 20, COL_KEY_BG);
        lcd_draw_rect(256, 4, 56, 20, COL_ACCENT);
        lcd_draw_text(272, 10, page_str, COL_WHITE, COL_KEY_BG, 1);
    }

    lcd_fill_rect(6, 34, 308, 178, COL_DISP_BG);
    lcd_draw_rect(6, 34, 308, 178, COL_ACCENT);

    int sl = -1, sc = -1, el = -1, ec = -1;
    if (ai_sel_state > 0) {
        get_ai_sel_bounds(&sl, &sc, &el, &ec);
    }

    int start_line = ai_current_page * LINES_PER_PAGE;
    int end_line = start_line + LINES_PER_PAGE;
    if (end_line > ai_line_count) end_line = ai_line_count;
    int y_pos = 44;

    for (int l = start_line; l < end_line; l++) {
        int ll = (int)strlen(ai_lines[l]);
        if (ll == 0) {
            if (l == ai_cur_line) {
                lcd_draw_char(14, y_pos, ' ', COL_BLACK, COL_ACCENT, 1);
            }
        } else {
            for (int c = 0; c < ll; c++) {
                bool is_cur = (l == ai_cur_line && c == ai_cur_col);
                bool is_sel = false;
                if (ai_sel_state > 0) {
                    if (l > sl && l < el) is_sel = true;
                    else if (l == sl && l == el && c >= sc && c <= ec) is_sel = true;
                    else if (l == sl && l < el && c >= sc) is_sel = true;
                    else if (l == el && l > sl && c <= ec) is_sel = true;
                }
                uint16_t fg = COL_WHITE;
                uint16_t bg = COL_DISP_BG;
                if (is_cur) {
                    fg = COL_BLACK;
                    bg = COL_ACCENT;
                } else if (is_sel) {
                    fg = COL_BLACK;
                    bg = COL_ANS;
                }
                lcd_draw_char(14 + c * 6, y_pos, ai_lines[l][c], fg, bg, 1);
            }
        }
        y_pos += 16;
    }

    lcd_fill_rect(0, 218, LCD_W, 22, COL_HEADER);
    if (ai_just_copied) {
        lcd_draw_text(16, 225, "Copied to clipboard!   [C] Close", COL_BADGE_DEG, COL_HEADER, 1);
    } else if (ai_sel_state == 1) {
        lcd_draw_text(6, 225, "SELECTING: [2/8/4/6] Move  [5] Lock  [0] Copy", COL_BADGE_DEG, COL_HEADER, 1);
    } else if (ai_sel_state == 2) {
        lcd_draw_text(8, 225, "SELECTED: [0] Copy  [5] Clear  [C] Exit", COL_ANS, COL_HEADER, 1);
    } else {
        lcd_draw_text(6, 225, "[2/8/4/6] Cur [5] Sel [0] Copy [*][#] Pg [C]Ex", COL_ANS, COL_HEADER, 1);
    }
}

void ai_overlay_show(const char *title, const char *body) {
    ai_overlay_active = true;
    strncpy(ai_title, title, sizeof(ai_title) - 1);
    ai_title[sizeof(ai_title) - 1] = '\0';
    paginate_ai_text(body);
    render_ai_page();
}

void ai_overlay_handle_key(char key) {
    if (key == '2') {
        if (ai_cur_line > 0) {
            ai_cur_line--;
            int ll = (int)strlen(ai_lines[ai_cur_line]);
            if (ai_cur_col >= ll) ai_cur_col = (ll > 0) ? (ll - 1) : 0;
            ai_current_page = ai_cur_line / LINES_PER_PAGE;
            ai_just_copied = false;
            render_ai_page();
        }
    } else if (key == '8') {
        if (ai_cur_line < ai_line_count - 1) {
            ai_cur_line++;
            int ll = (int)strlen(ai_lines[ai_cur_line]);
            if (ai_cur_col >= ll) ai_cur_col = (ll > 0) ? (ll - 1) : 0;
            ai_current_page = ai_cur_line / LINES_PER_PAGE;
            ai_just_copied = false;
            render_ai_page();
        }
    } else if (key == '4') {
        if (ai_cur_col > 0) {
            ai_cur_col--;
            ai_just_copied = false;
            render_ai_page();
        } else if (ai_cur_line > 0) {
            ai_cur_line--;
            int ll = (int)strlen(ai_lines[ai_cur_line]);
            ai_cur_col = (ll > 0) ? (ll - 1) : 0;
            ai_current_page = ai_cur_line / LINES_PER_PAGE;
            ai_just_copied = false;
            render_ai_page();
        }
    } else if (key == '6') {
        int ll = (int)strlen(ai_lines[ai_cur_line]);
        if (ai_cur_col < ll - 1) {
            ai_cur_col++;
            ai_just_copied = false;
            render_ai_page();
        } else if (ai_cur_line < ai_line_count - 1) {
            ai_cur_line++;
            ai_cur_col = 0;
            ai_current_page = ai_cur_line / LINES_PER_PAGE;
            ai_just_copied = false;
            render_ai_page();
        }
    } else if (key == '5') {
        if (ai_sel_state == 0) {
            ai_mark_line = ai_cur_line;
            ai_mark_col = ai_cur_col;
            ai_sel_state = 1;
        } else if (ai_sel_state == 1) {
            ai_end_line = ai_cur_line;
            ai_end_col = ai_cur_col;
            ai_sel_state = 2;
        } else {
            ai_sel_state = 0;
        }
        ai_just_copied = false;
        render_ai_page();
    } else if (key == '*') {
        if (ai_current_page > 0) {
            ai_current_page--;
            ai_cur_line = ai_current_page * LINES_PER_PAGE;
            ai_cur_col = 0;
            ai_just_copied = false;
            render_ai_page();
        }
    } else if (key == '#') {
        if (ai_current_page < ai_total_pages - 1) {
            ai_current_page++;
            ai_cur_line = ai_current_page * LINES_PER_PAGE;
            ai_cur_col = 0;
            ai_just_copied = false;
            render_ai_page();
        }
    } else if (key == '0') {
        char buf[MAX_EXPR_LEN];
        int pos = 0;
        buf[0] = '\0';

        if (ai_sel_state > 0) {
            int sl, sc, el, ec;
            get_ai_sel_bounds(&sl, &sc, &el, &ec);
            for (int l = sl; l <= el && l < ai_line_count; l++) {
                int ll = (int)strlen(ai_lines[l]);
                if (ll == 0) continue;
                int c0 = (l == sl) ? sc : 0;
                int c1 = (l == el) ? ec : (ll - 1);
                if (c0 < 0) c0 = 0;
                if (c1 >= ll) c1 = ll - 1;
                if (c0 > c1) continue;
                if (l > sl && pos > 0 && pos < MAX_EXPR_LEN - 1) {
                    buf[pos++] = ' ';
                }
                int cnt = c1 - c0 + 1;
                if (pos + cnt >= MAX_EXPR_LEN - 1) {
                    cnt = (MAX_EXPR_LEN - 1) - pos;
                }
                if (cnt > 0) {
                    memcpy(buf + pos, ai_lines[l] + c0, cnt);
                    pos += cnt;
                }
                if (pos >= MAX_EXPR_LEN - 1) break;
            }
            buf[pos] = '\0';
        } else if (ai_cur_line >= 0 && ai_cur_line < ai_line_count) {
            strncpy(buf, ai_lines[ai_cur_line], MAX_EXPR_LEN - 1);
            buf[MAX_EXPR_LEN - 1] = '\0';
        }
        calc_set_clipboard(buf);
        ai_sel_state = 0;
        ai_just_copied = true;
        render_ai_page();
    } else if (key == 'C') {
        ai_overlay_active = false;
        ai_sel_state = 0;
        ai_just_copied = false;
        calc_draw_ui();
        calc_mark_dirty();
    }
}
