#include "graph_ui.h"
#include "app_config.h"
#include "lcd.h"
#include "calc_ui.h"
#include "math_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static const uint16_t graph_colors[5] = {
    COL_ANS,
    COL_ACCENT,
    COL_BADGE_DEG,
    0xF81F,
    0xFFE0
};

static bool graph_active = false;
static double graph_xmin = -10.0;
static double graph_xmax = 10.0;
static double graph_ymin = -6.0;
static double graph_ymax = 6.0;

bool graph_is_active(void) {
    return graph_active;
}

static double get_nice_step(double range, int target_ticks, int *out_subdivs) {
    double raw = range / (double)target_ticks;
    if (raw <= 1e-9) {
        *out_subdivs = 5;
        return 1.0;
    }
    double mag = pow(10.0, floor(log10(raw)));
    double norm = raw / mag;
    if (norm < 1.5) {
        *out_subdivs = 5;
        return 1.0 * mag;
    }
    if (norm < 3.5) {
        *out_subdivs = 4;
        return 2.0 * mag;
    }
    if (norm < 7.5) {
        *out_subdivs = 5;
        return 5.0 * mag;
    }
    *out_subdivs = 5;
    return 10.0 * mag;
}

static void format_tick_label(double val, double step, char *out, size_t out_sz) {
    if (fabs(val) < step * 0.001) {
        snprintf(out, out_sz, "0");
        return;
    }
    if (step >= 1.0 - 1e-6) {
        snprintf(out, out_sz, "%.0f", val);
    } else if (step >= 0.1 - 1e-6) {
        snprintf(out, out_sz, "%.1f", val);
    } else {
        snprintf(out, out_sz, "%.2f", val);
    }
}

static void render_graph(void) {
    const char *expression = calc_get_expression();
    int expr_len = calc_get_expr_len();

    int plot_y0 = 29;
    int plot_h = 189;
    int plot_y1 = plot_y0 + plot_h - 1;

    lcd_fill_rect(0, 0, LCD_W, 28, COL_HEADER);
    lcd_fill_rect(0, 28, LCD_W, 1, COL_ACCENT);

    char hdr[48];
    snprintf(hdr, sizeof(hdr), "y=%.30s", expression);
    lcd_draw_text(8, 10, hdr, COL_ACCENT, COL_HEADER, 1);

    char rng[64];
    snprintf(rng, sizeof(rng), "X[%.1f,%.1f]", graph_xmin, graph_xmax);
    lcd_draw_text(210, 10, rng, COL_WHITE, COL_HEADER, 1);

    lcd_fill_rect(0, plot_y0, LCD_W, plot_h, COL_DISP_BG);

    int x_axis_py = -1;
    int y_axis_px = -1;
    if (graph_ymin <= 0.0 && graph_ymax >= 0.0) {
        x_axis_py = plot_y0 + (int)round((graph_ymax / (graph_ymax - graph_ymin)) * (plot_h - 1));
        if (x_axis_py >= plot_y0 && x_axis_py <= plot_y1) {
            lcd_fill_rect(0, x_axis_py, LCD_W, 1, COL_AXIS);
        }
    }
    if (graph_xmin <= 0.0 && graph_xmax >= 0.0) {
        y_axis_px = (int)round((-graph_xmin / (graph_xmax - graph_xmin)) * (LCD_W - 1));
        if (y_axis_px >= 0 && y_axis_px < LCD_W) {
            lcd_fill_rect(y_axis_px, plot_y0, 1, plot_h, COL_AXIS);
        }
    }

    int x_sub = 5;
    double x_step = get_nice_step(graph_xmax - graph_xmin, 6, &x_sub);
    double x_minor = x_step / (double)x_sub;

    int x_lbl_y = plot_y1 - 9;
    if (x_axis_py >= plot_y0 && x_axis_py <= plot_y1) {
        x_lbl_y = (x_axis_py + 12 <= plot_y1) ? (x_axis_py + 4) : (x_axis_py - 10);
    }

    if (x_minor > 0.0 && x_axis_py >= plot_y0 && x_axis_py <= plot_y1) {
        double mx = ceil(graph_xmin / x_minor) * x_minor;
        while (mx <= graph_xmax + x_minor * 0.01) {
            int px = (int)round(((mx - graph_xmin) / (graph_xmax - graph_xmin)) * (LCD_W - 1));
            if (px >= 0 && px < LCD_W) {
                lcd_fill_rect(px, x_axis_py - 1, 1, 3, COL_AXIS);
            }
            mx += x_minor;
        }
    }

    double tx = ceil(graph_xmin / x_step) * x_step;
    while (tx <= graph_xmax + x_step * 0.01) {
        int px = (int)round(((tx - graph_xmin) / (graph_xmax - graph_xmin)) * (LCD_W - 1));
        if (px >= 0 && px < LCD_W) {
            if (x_axis_py >= plot_y0 && x_axis_py <= plot_y1) {
                lcd_fill_rect(px, x_axis_py - 3, 1, 7, COL_WHITE);
            }
            if (!(fabs(tx) < x_step * 0.001 && y_axis_px >= 0 && y_axis_px < LCD_W)) {
                char lbl[24];
                format_tick_label(tx, x_step, lbl, sizeof(lbl));
                int tw = (int)strlen(lbl) * 6;
                int lx = px - tw / 2;
                if (lx >= 2 && lx + tw <= LCD_W - 2) {
                    lcd_draw_text(lx, x_lbl_y, lbl, COL_TICK_TXT, COL_DISP_BG, 1);
                }
            }
        }
        tx += x_step;
    }

    int y_sub = 5;
    double y_step = get_nice_step(graph_ymax - graph_ymin, 4, &y_sub);
    double y_minor = y_step / (double)y_sub;

    int y_lbl_x = 4;
    if (y_axis_px >= 0 && y_axis_px < LCD_W) {
        y_lbl_x = (y_axis_px + 34 < LCD_W) ? (y_axis_px + 4) : (y_axis_px - 32);
    }

    if (y_minor > 0.0 && y_axis_px >= 0 && y_axis_px < LCD_W) {
        double my = ceil(graph_ymin / y_minor) * y_minor;
        while (my <= graph_ymax + y_minor * 0.01) {
            int py = plot_y0 + (int)round(((graph_ymax - my) / (graph_ymax - graph_ymin)) * (plot_h - 1));
            if (py >= plot_y0 && py <= plot_y1) {
                lcd_fill_rect(y_axis_px - 1, py, 3, 1, COL_AXIS);
            }
            my += y_minor;
        }
    }

    double ty = ceil(graph_ymin / y_step) * y_step;
    while (ty <= graph_ymax + y_step * 0.01) {
        int py = plot_y0 + (int)round(((graph_ymax - ty) / (graph_ymax - graph_ymin)) * (plot_h - 1));
        if (py >= plot_y0 && py <= plot_y1) {
            if (y_axis_px >= 0 && y_axis_px < LCD_W) {
                lcd_fill_rect(y_axis_px - 3, py, 7, 1, COL_WHITE);
            }
            if (!(fabs(ty) < y_step * 0.001 && x_axis_py >= plot_y0 && x_axis_py <= plot_y1)) {
                char lbl[24];
                format_tick_label(ty, y_step, lbl, sizeof(lbl));
                int ly = py - 3;
                if (ly >= plot_y0 + 1 && ly + 7 <= plot_y1 - 1) {
                    lcd_draw_text(y_lbl_x, ly, lbl, COL_TICK_TXT, COL_DISP_BG, 1);
                }
            }
        }
        ty += y_step;
    }

    int seg_start[5], seg_end[5];
    int num_graphs = 0;
    int i = 0;
    while (i < expr_len && num_graphs < 5) {
        while (i < expr_len && expression[i] == ' ') i++;
        if (i >= expr_len) break;
        int st = i;
        int d = 0;
        bool in_abs = false;
        while (i < expr_len) {
            char c = expression[i];
            if (c == '|') in_abs = !in_abs;
            else if (c == '(') d++;
            else if (c == ')') d--;
            else if (c == ' ' && d == 0 && !in_abs) break;
            i++;
        }
        seg_start[num_graphs] = st;
        seg_end[num_graphs] = i;
        num_graphs++;
    }

    for (int g = 0; g < num_graphs; g++) {
        uint16_t col = graph_colors[g % 5];
        int prev_py = -1;
        bool prev_valid = false;

        for (int px = 0; px < LCD_W; px++) {
            double x_val = graph_xmin + ((double)px / (double)(LCD_W - 1)) * (graph_xmax - graph_xmin);
            math_set_eval_x(true, x_val);
            double yv = 0.0;
            bool ok = math_parse_and_calculate(expression, seg_start[g], seg_end[g], &yv);
            if (ok && !isnan(yv) && !isinf(yv)) {
                int py = plot_y0 + (int)round(((graph_ymax - yv) / (graph_ymax - graph_ymin)) * (plot_h - 1));
                if (py >= plot_y0 && py <= plot_y1) {
                    if (prev_valid && abs(py - prev_py) < 120) {
                        int y_start = prev_py < py ? prev_py : py;
                        int y_end = prev_py > py ? prev_py : py;
                        lcd_fill_rect(px, y_start, 2, y_end - y_start + 1, col);
                    } else {
                        lcd_fill_rect(px, py, 2, 2, col);
                    }
                    prev_py = py;
                    prev_valid = true;
                } else {
                    prev_valid = false;
                }
            } else {
                prev_valid = false;
            }
        }
    }
    math_set_eval_x(false, 0.0);

    lcd_fill_rect(0, 218, LCD_W, 22, COL_HEADER);
    lcd_draw_text(6, 225, "[2/8/4/6] Pan  [*][#] Zoom  [5] Rst  [C] Exit", COL_ANS, COL_HEADER, 1);
}

void graph_start_mode(void) {
    if (calc_get_expr_len() == 0) {
        calc_clear_result_area();
        lcd_draw_text(14, 96, "Enter f(x) first!", COL_RED, COL_DISP_BG, 2);
        return;
    }
    graph_xmin = -10.0;
    graph_xmax = 10.0;
    graph_ymin = -6.0;
    graph_ymax = 6.0;
    graph_active = true;
    render_graph();
}

void graph_handle_key(char key) {
    double dx = (graph_xmax - graph_xmin) * 0.2;
    double dy = (graph_ymax - graph_ymin) * 0.2;
    double cx = (graph_xmin + graph_xmax) * 0.5;
    double cy = (graph_ymin + graph_ymax) * 0.5;
    double hw = (graph_xmax - graph_xmin) * 0.5;
    double hh = (graph_ymax - graph_ymin) * 0.5;

    if (key == '2') {
        graph_ymin += dy;
        graph_ymax += dy;
        render_graph();
    } else if (key == '8') {
        graph_ymin -= dy;
        graph_ymax -= dy;
        render_graph();
    } else if (key == '4') {
        graph_xmin -= dx;
        graph_xmax -= dx;
        render_graph();
    } else if (key == '6') {
        graph_xmin += dx;
        graph_xmax += dx;
        render_graph();
    } else if (key == '#') {
        graph_xmin = cx - hw * 0.7;
        graph_xmax = cx + hw * 0.7;
        graph_ymin = cy - hh * 0.7;
        graph_ymax = cy + hh * 0.7;
        render_graph();
    } else if (key == '*') {
        graph_xmin = cx - hw * 1.4;
        graph_xmax = cx + hw * 1.4;
        graph_ymin = cy - hh * 1.4;
        graph_ymax = cy + hh * 1.4;
        render_graph();
    } else if (key == '5') {
        graph_xmin = -10.0;
        graph_xmax = 10.0;
        graph_ymin = -6.0;
        graph_ymax = 6.0;
        render_graph();
    } else {
        graph_active = false;
        calc_draw_ui();
        calc_mark_dirty();
    }
}
