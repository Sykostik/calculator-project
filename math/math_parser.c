#include "math_parser.h"
#include "app_config.h"
#include "lcd.h"
#include "calc_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static bool is_deg_mode = true;
static double last_answer = 0.0;
static bool eval_use_x = false;
static double graph_x_val = 0.0;

bool math_is_deg_mode(void) {
    return is_deg_mode;
}

void math_toggle_deg_mode(void) {
    is_deg_mode = !is_deg_mode;
}

void math_set_eval_x(bool enabled, double x_val) {
    eval_use_x = enabled;
    graph_x_val = x_val;
}

static bool degree_to_pi_string(double deg, char *out, size_t out_sz) {
    double tol = 0.001;
    const char *sign = "";
    if (deg < -tol) {
        sign = "-";
        deg = -deg;
    }
    if (fabs(deg - 0.0) < tol)   { snprintf(out, out_sz, "0"); return true; }
    if (fabs(deg - 30.0) < tol)  { snprintf(out, out_sz, "%sPI/6", sign); return true; }
    if (fabs(deg - 45.0) < tol)  { snprintf(out, out_sz, "%sPI/4", sign); return true; }
    if (fabs(deg - 60.0) < tol)  { snprintf(out, out_sz, "%sPI/3", sign); return true; }
    if (fabs(deg - 90.0) < tol)  { snprintf(out, out_sz, "%sPI/2", sign); return true; }
    if (fabs(deg - 120.0) < tol) { snprintf(out, out_sz, "%s2PI/3", sign); return true; }
    if (fabs(deg - 135.0) < tol) { snprintf(out, out_sz, "%s3PI/4", sign); return true; }
    if (fabs(deg - 150.0) < tol) { snprintf(out, out_sz, "%s5PI/6", sign); return true; }
    if (fabs(deg - 180.0) < tol) { snprintf(out, out_sz, "%sPI", sign); return true; }
    if (fabs(deg - 210.0) < tol) { snprintf(out, out_sz, "%s7PI/6", sign); return true; }
    if (fabs(deg - 225.0) < tol) { snprintf(out, out_sz, "%s5PI/4", sign); return true; }
    if (fabs(deg - 240.0) < tol) { snprintf(out, out_sz, "%s4PI/3", sign); return true; }
    if (fabs(deg - 270.0) < tol) { snprintf(out, out_sz, "%s3PI/2", sign); return true; }
    if (fabs(deg - 300.0) < tol) { snprintf(out, out_sz, "%s5PI/3", sign); return true; }
    if (fabs(deg - 315.0) < tol) { snprintf(out, out_sz, "%s7PI/4", sign); return true; }
    if (fabs(deg - 330.0) < tol) { snprintf(out, out_sz, "%s11PI/6", sign); return true; }
    if (fabs(deg - 360.0) < tol) { snprintf(out, out_sz, "%s2PI", sign); return true; }
    out[0] = '\0';
    return false;
}

static void format_symbolic_result(double val, char *out, size_t out_sz) {
    double tol = 0.0001;
    if (fabs(val) < tol) {
        snprintf(out, out_sz, "0");
        return;
    }

    const char *sign = "";
    double abs_val = val;
    if (val < 0) {
        sign = "-";
        abs_val = -val;
    }

    double deg_equiv = abs_val * (180.0 / M_PI);
    char pi_str[32];
    if (degree_to_pi_string(deg_equiv, pi_str, sizeof(pi_str)) && strcmp(pi_str, "0") != 0) {
        snprintf(out, out_sz, "%s%s", sign, pi_str);
        return;
    }

    if (fabs(abs_val - 1.0) < tol)               { snprintf(out, out_sz, "%s1", sign); return; }
    if (fabs(abs_val - 0.5) < tol)               { snprintf(out, out_sz, "%s1/2", sign); return; }
    if (fabs(abs_val - (sqrt(2.0) / 2.0)) < tol) { snprintf(out, out_sz, "%ssqrt(2)/2", sign); return; }
    if (fabs(abs_val - (sqrt(3.0) / 2.0)) < tol) { snprintf(out, out_sz, "%ssqrt(3)/2", sign); return; }
    if (fabs(abs_val - (sqrt(3.0) / 3.0)) < tol) { snprintf(out, out_sz, "%ssqrt(3)/3", sign); return; }
    if (fabs(abs_val - sqrt(3.0)) < tol)         { snprintf(out, out_sz, "%ssqrt(3)", sign); return; }

    if (fabs(val - round(val)) < tol && fabs(val) < 2000000000.0) {
        snprintf(out, out_sz, "%ld", (long)round(val));
        return;
    }
    snprintf(out, out_sz, "%.4f", val);
}

static bool get_trig_angle_pi(const char *expr, int len, char *out, size_t out_sz) {
    if (len > 5 && expr[len - 1] == ')') {
        if (strncmp(expr, "sin(", 4) == 0 || strncmp(expr, "cos(", 4) == 0 ||
            strncmp(expr, "tan(", 4) == 0 || strncmp(expr, "ctg(", 4) == 0) {
            double angle_val = 0;
            if (math_parse_and_calculate(expr, 4, len - 1, &angle_val)) {
                return degree_to_pi_string(angle_val, out, out_sz);
            }
        }
    }
    out[0] = '\0';
    return false;
}

static bool is_inverse_trig(const char *expr, int len) {
    if (len > 6 && expr[len - 1] == ')') {
        if (strncmp(expr, "asin(", 5) == 0 || strncmp(expr, "acos(", 5) == 0 ||
            strncmp(expr, "atan(", 5) == 0 || strncmp(expr, "actg(", 5) == 0) {
            return true;
        }
    }
    return false;
}

static bool has_var_x(const char *s, int start, int end) {
    for (int i = start; i < end; i++) {
        if (s[i] == 'x') return true;
    }
    return false;
}

static void format_num_short(double v, char *out, size_t out_sz) {
    if (fabs(v - round(v)) < 1e-6 && fabs(v) < 1e9) {
        snprintf(out, out_sz, "%ld", (long)round(v));
    } else {
        snprintf(out, out_sz, "%.2f", v);
    }
}

bool math_symbolic_diff(const char *str, int start, int end, char *out, size_t out_sz) {
    while (start < end && str[start] == ' ') start++;
    while (end > start && str[end - 1] == ' ') end--;
    if (start >= end) return false;

    if (str[start] == '(' && str[end - 1] == ')') {
        int depth = 0;
        bool wrapped = true;
        for (int i = start; i < end - 1; i++) {
            if (str[i] == '(') depth++;
            else if (str[i] == ')') depth--;
            if (depth == 0) {
                wrapped = false;
                break;
            }
        }
        if (wrapped) return math_symbolic_diff(str, start + 1, end - 1, out, out_sz);
    }

    if (!has_var_x(str, start, end)) {
        snprintf(out, out_sz, "0");
        return true;
    }

    int depth = 0;
    bool in_abs = false;
    for (int i = end - 1; i >= start; i--) {
        char c = str[i];
        if (c == '|') { in_abs = !in_abs; continue; }
        if (c == ')') depth++;
        else if (c == '(') depth--;
        if (depth == 0 && !in_abs &&
            (c == '+' || (c == '-' && i > start && str[i - 1] != '*' && str[i - 1] != '/' && str[i - 1] != '^'))) {
            char ld[48], rd[48];
            if (!math_symbolic_diff(str, start, i, ld, sizeof(ld))) return false;
            if (!math_symbolic_diff(str, i + 1, end, rd, sizeof(rd))) return false;
            if (strcmp(ld, "0") == 0 && strcmp(rd, "0") == 0) {
                snprintf(out, out_sz, "0");
            } else if (strcmp(rd, "0") == 0) {
                snprintf(out, out_sz, "%.40s", ld);
            } else if (strcmp(ld, "0") == 0) {
                if (c == '-') snprintf(out, out_sz, "-%.40s", rd);
                else snprintf(out, out_sz, "%.40s", rd);
            } else {
                snprintf(out, out_sz, "%.24s%c%.24s", ld, c, rd);
            }
            return true;
        }
    }

    if (str[start] == '-') {
        char sub[48];
        if (!math_symbolic_diff(str, start + 1, end, sub, sizeof(sub))) return false;
        if (strcmp(sub, "0") == 0) snprintf(out, out_sz, "0");
        else if (sub[0] == '-') snprintf(out, out_sz, "%.40s", sub + 1);
        else snprintf(out, out_sz, "-%.40s", sub);
        return true;
    }

    int len = end - start;
    if (len == 1 && str[start] == 'x') {
        snprintf(out, out_sz, "1");
        return true;
    }

    depth = 0;
    in_abs = false;
    for (int i = end - 1; i >= start; i--) {
        if (str[i] == '|') { in_abs = !in_abs; continue; }
        if (str[i] == ')') depth++;
        else if (str[i] == '(') depth--;
        if (depth == 0 && !in_abs && str[i] == '*') {
            bool lx = has_var_x(str, start, i);
            bool rx = has_var_x(str, i + 1, end);
            if (!lx && rx) {
                double cval = 0;
                if (!math_parse_and_calculate(str, start, i, &cval)) return false;
                int rstart = i + 1;
                while (rstart < end && str[rstart] == ' ') rstart++;
                if (end - rstart == 1 && str[rstart] == 'x') {
                    format_num_short(cval, out, out_sz);
                    return true;
                }
                int pow_idx = -1;
                int d2 = 0;
                for (int k = rstart; k < end; k++) {
                    if (str[k] == '(') d2++;
                    else if (str[k] == ')') d2--;
                    else if (d2 == 0 && str[k] == '^') { pow_idx = k; break; }
                }
                if (pow_idx == rstart + 1 && str[rstart] == 'x' && !has_var_x(str, pow_idx + 1, end)) {
                    double n = 0;
                    if (math_parse_and_calculate(str, pow_idx + 1, end, &n)) {
                        double coeff = cval * n;
                        double new_n = n - 1.0;
                        char cb[24], nb[24];
                        format_num_short(coeff, cb, sizeof(cb));
                        format_num_short(new_n, nb, sizeof(nb));
                        if (fabs(new_n) < 1e-6) snprintf(out, out_sz, "%.20s", cb);
                        else if (fabs(new_n - 1.0) < 1e-6) snprintf(out, out_sz, "%.20s*x", cb);
                        else snprintf(out, out_sz, "%.20s*x^%.16s", cb, nb);
                        return true;
                    }
                }
                char rd[48], cb[24];
                if (!math_symbolic_diff(str, i + 1, end, rd, sizeof(rd))) return false;
                format_num_short(cval, cb, sizeof(cb));
                snprintf(out, out_sz, "%.16s*%.30s", cb, rd);
                return true;
            }
        }
    }

    depth = 0;
    in_abs = false;
    for (int i = end - 1; i >= start; i--) {
        if (str[i] == '|') { in_abs = !in_abs; continue; }
        if (str[i] == ')') depth++;
        else if (str[i] == '(') depth--;
        if (depth == 0 && !in_abs && str[i] == '^') {
            int lstart = start, lend = i;
            while (lstart < lend && str[lstart] == ' ') lstart++;
            while (lend > lstart && str[lend - 1] == ' ') lend--;
            if (lend - lstart == 1 && str[lstart] == 'x' && !has_var_x(str, i + 1, end)) {
                double n = 0;
                if (!math_parse_and_calculate(str, i + 1, end, &n)) return false;
                if (fabs(n) < 1e-6) {
                    snprintf(out, out_sz, "0");
                } else if (fabs(n - 1.0) < 1e-6) {
                    snprintf(out, out_sz, "1");
                } else if (fabs(n - 2.0) < 1e-6) {
                    snprintf(out, out_sz, "2*x");
                } else {
                    char nb[24], mb[24];
                    format_num_short(n, nb, sizeof(nb));
                    format_num_short(n - 1.0, mb, sizeof(mb));
                    snprintf(out, out_sz, "%.16s*x^%.16s", nb, mb);
                }
                return true;
            }
        }
    }

    if (len == 3 && strncmp(str + start, "|x|", 3) == 0)     { snprintf(out, out_sz, "x/|x|"); return true; }
    if (len == 6 && strncmp(str + start, "abs(x)", 6) == 0)  { snprintf(out, out_sz, "x/|x|"); return true; }
    if (len == 6 && strncmp(str + start, "sin(x)", 6) == 0)  { snprintf(out, out_sz, "cos(x)"); return true; }
    if (len == 6 && strncmp(str + start, "cos(x)", 6) == 0)  { snprintf(out, out_sz, "-sin(x)"); return true; }
    if (len == 6 && strncmp(str + start, "tan(x)", 6) == 0)  { snprintf(out, out_sz, "1/cos(x)^2"); return true; }
    if (len == 5 && strncmp(str + start, "ln(x)", 5) == 0)   { snprintf(out, out_sz, "1/x"); return true; }
    if (len == 6 && strncmp(str + start, "exp(x)", 6) == 0)  { snprintf(out, out_sz, "exp(x)"); return true; }
    if (len == 7 && strncmp(str + start, "sqrt(x)", 7) == 0) { snprintf(out, out_sz, "1/(2*sqrt(x))"); return true; }

    return false;
}

bool math_parse_and_calculate(const char *str, int start, int end, double *out_res) {
    while (start < end && str[start] == ' ') start++;
    while (end > start && str[end - 1] == ' ') end--;
    if (start >= end) return false;

    if (str[end - 1] == DEG_CHAR) {
        return math_parse_and_calculate(str, start, end - 1, out_res);
    }

    if (str[start] == '(' && str[end - 1] == ')') {
        int depth = 0;
        bool wrapped = true;
        for (int i = start; i < end - 1; i++) {
            if (str[i] == '(') depth++;
            else if (str[i] == ')') depth--;
            if (depth == 0) {
                wrapped = false;
                break;
            }
        }
        if (wrapped) {
            return math_parse_and_calculate(str, start + 1, end - 1, out_res);
        }
    }

    int idx = -1;
    char op = ' ';
    int depth = 0;
    bool in_abs = false;

    for (int i = end - 1; i >= start; i--) {
        char c = str[i];
        if (c == '|') { in_abs = !in_abs; continue; }
        if (c == ')') depth++;
        else if (c == '(') depth--;
        if (depth == 0 && !in_abs) {
            if (c == '+' || (c == '-' && i > start && str[i - 1] != 'e' && str[i - 1] != 'E' &&
                             str[i - 1] != '*' && str[i - 1] != '/' && str[i - 1] != '^' &&
                             str[i - 1] != '+' && str[i - 1] != '-')) {
                op = c;
                idx = i;
                break;
            }
        }
    }

    if (idx == -1) {
        depth = 0;
        in_abs = false;
        for (int i = end - 1; i >= start; i--) {
            char c = str[i];
            if (c == '|') { in_abs = !in_abs; continue; }
            if (c == ')') depth++;
            else if (c == '(') depth--;
            if (depth == 0 && !in_abs && (c == '*' || c == '/' || c == '%')) {
                if (c == '/' && i >= start + 1 && i + 2 < end &&
                    str[i - 1] == 'd' && str[i + 1] == 'd' && str[i + 2] == 'x') {
                    continue;
                }
                op = c;
                idx = i;
                break;
            }
        }
    }

    if (idx == -1) {
        depth = 0;
        in_abs = false;
        for (int i = end - 1; i >= start; i--) {
            char c = str[i];
            if (c == '|') { in_abs = !in_abs; continue; }
            if (c == ')') depth++;
            else if (c == '(') depth--;
            if (depth == 0 && !in_abs && c == '^') {
                op = c;
                idx = i;
                break;
            }
        }
    }

    if (idx != -1) {
        double lval = 0, rval = 0;
        if (!math_parse_and_calculate(str, start, idx, &lval)) return false;
        if (!math_parse_and_calculate(str, idx + 1, end, &rval)) return false;
        if (op == '+') *out_res = lval + rval;
        else if (op == '-') *out_res = lval - rval;
        else if (op == '*') *out_res = lval * rval;
        else if (op == '/') {
            if (rval == 0.0) return false;
            *out_res = lval / rval;
        } else if (op == '%') {
            if (rval == 0.0) return false;
            *out_res = fmod(lval, rval);
        } else if (op == '^') {
            *out_res = pow(lval, rval);
        }
        return true;
    }

    if (str[start] == '|' && str[end - 1] == '|' && end - start >= 3) {
        double inner = 0;
        if (!math_parse_and_calculate(str, start + 1, end - 1, &inner)) return false;
        *out_res = fabs(inner);
        return true;
    }

    if (str[start] == '-') {
        double val = 0;
        if (!math_parse_and_calculate(str, start + 1, end, &val)) return false;
        *out_res = -val;
        return true;
    }

    int len = end - start;
    if (len == 1 && str[start] == 'x') {
        if (!eval_use_x) return false;
        *out_res = graph_x_val;
        return true;
    }
    if (len == 2 && strncmp(str + start, "pi", 2) == 0)  { *out_res = M_PI; return true; }
    if (len == 1 && str[start] == 'e')                    { *out_res = 2.71828182845904523536; return true; }
    if (len == 3 && strncmp(str + start, "Ans", 3) == 0) { *out_res = last_answer; return true; }

    if (str[end - 1] == ')') {
        int paren = -1;
        for (int i = start; i < end; i++) {
            if (str[i] == '(') {
                paren = i;
                break;
            }
        }
        if (paren > start) {
            int fn_end = paren;
            while (fn_end > start && str[fn_end - 1] == ' ') fn_end--;
            int fn_len = fn_end - start;
            if (fn_len == 4 && strncmp(str + start, "d/dx", 4) == 0) {
                if (!eval_use_x) return false;
                double orig_x = graph_x_val;
                double h = 1e-4;
                double y1 = 0.0, y2 = 0.0;
                graph_x_val = orig_x + h;
                bool ok1 = math_parse_and_calculate(str, paren + 1, end - 1, &y1);
                graph_x_val = orig_x - h;
                bool ok2 = math_parse_and_calculate(str, paren + 1, end - 1, &y2);
                graph_x_val = orig_x;
                if (!ok1 || !ok2) return false;
                *out_res = (y1 - y2) / (2.0 * h);
                return true;
            }

            double val = 0;
            if (!math_parse_and_calculate(str, paren + 1, end - 1, &val)) return false;

            if (fn_len == 4 && strncmp(str + start, "sqrt", 4) == 0) { if (val < 0) return false; *out_res = sqrt(val); return true; }
            if (fn_len == 4 && strncmp(str + start, "cbrt", 4) == 0) { *out_res = cbrt(val); return true; }
            if (fn_len == 3 && strncmp(str + start, "log", 3) == 0)  { if (val <= 0) return false; *out_res = log10(val); return true; }
            if (fn_len > 3 && strncmp(str + start, "log", 3) == 0) {
                double base = 0;
                if (!math_parse_and_calculate(str, start + 3, fn_end, &base)) return false;
                if (val <= 0 || base <= 0 || fabs(base - 1.0) < 1e-9) return false;
                *out_res = log(val) / log(base);
                return true;
            }
            if (fn_len == 2 && strncmp(str + start, "ln", 2) == 0)   { if (val <= 0) return false; *out_res = log(val); return true; }
            if (fn_len == 3 && strncmp(str + start, "exp", 3) == 0)  { *out_res = exp(val); return true; }
            if (fn_len == 3 && strncmp(str + start, "abs", 3) == 0)  { *out_res = fabs(val); return true; }
            if (fn_len == 4 && strncmp(str + start, "fact", 4) == 0) {
                if (val < 0 || val > 170 || fabs(val - round(val)) > 0.0001) return false;
                int n = (int)round(val);
                double f = 1.0;
                for (int k = 2; k <= n; k++) f *= k;
                *out_res = f;
                return true;
            }
            double rad_val = is_deg_mode ? (val * M_PI / 180.0) : val;
            if (fn_len == 3 && strncmp(str + start, "sin", 3) == 0) { *out_res = sin(rad_val); return true; }
            if (fn_len == 3 && strncmp(str + start, "cos", 3) == 0) { *out_res = cos(rad_val); return true; }
            if (fn_len == 3 && strncmp(str + start, "tan", 3) == 0) {
                if (fabs(cos(rad_val)) < 0.0001) return false;
                *out_res = tan(rad_val);
                return true;
            }
            if (fn_len == 3 && strncmp(str + start, "ctg", 3) == 0) {
                if (fabs(sin(rad_val)) < 0.0001) return false;
                *out_res = 1.0 / tan(rad_val);
                return true;
            }
            if (fn_len == 4 && strncmp(str + start, "asin", 4) == 0) {
                if (val < -1.0 || val > 1.0) return false;
                *out_res = is_deg_mode ? asin(val) * 180.0 / M_PI : asin(val);
                return true;
            }
            if (fn_len == 4 && strncmp(str + start, "acos", 4) == 0) {
                if (val < -1.0 || val > 1.0) return false;
                *out_res = is_deg_mode ? acos(val) * 180.0 / M_PI : acos(val);
                return true;
            }
            if (fn_len == 4 && strncmp(str + start, "atan", 4) == 0) {
                *out_res = is_deg_mode ? atan(val) * 180.0 / M_PI : atan(val);
                return true;
            }
            if (fn_len == 4 && strncmp(str + start, "actg", 4) == 0) {
                double r = (val == 0.0) ? (M_PI / 2.0) : atan(1.0 / val);
                *out_res = is_deg_mode ? r * 180.0 / M_PI : r;
                return true;
            }
        }
    }

    char num_buf[32];
    if (len >= (int)sizeof(num_buf)) len = sizeof(num_buf) - 1;
    memcpy(num_buf, str + start, len);
    num_buf[len] = '\0';
    *out_res = atof(num_buf);
    return true;
}

void math_evaluate_expression(const char *expr, int expr_len) {
    calc_clear_result_area();

    int s = 0, e = expr_len;
    while (s < e && expr[s] == ' ') s++;
    while (e > s && expr[e - 1] == ' ') e--;
    if (e - s >= 6 && strncmp(expr + s, "d/dx", 4) == 0 && expr[e - 1] == ')') {
        int p = s + 4;
        while (p < e && expr[p] == ' ') p++;
        if (p < e && expr[p] == '(') {
            char diff_out[64];
            if (math_symbolic_diff(expr, p + 1, e - 1, diff_out, sizeof(diff_out))) {
                char line[80];
                snprintf(line, sizeof(line), "= %.40s", diff_out);
                int sc = (strlen(line) > 22) ? 1 : 2;
                lcd_draw_text(14, 96, line, COL_ANS, COL_DISP_BG, sc);
                return;
            }
        }
    }

    double result = 0;
    bool ok = math_parse_and_calculate(expr, 0, expr_len, &result);

    if (ok && !isnan(result) && !isinf(result)) {
        last_answer = result;
        char sym_ans[48];
        char rad_angle[32];
        char line[96];

        format_symbolic_result(result, sym_ans, sizeof(sym_ans));
        bool has_rad = get_trig_angle_pi(expr, expr_len, rad_angle, sizeof(rad_angle));

        if (is_deg_mode && is_inverse_trig(expr, expr_len)) {
            size_t sl = strlen(sym_ans);
            if (sl < sizeof(sym_ans) - 2) {
                sym_ans[sl] = DEG_CHAR;
                sym_ans[sl + 1] = '\0';
            }
        }

        if (!has_rad && !is_deg_mode) {
            has_rad = degree_to_pi_string(result, rad_angle, sizeof(rad_angle));
        }

        if (!is_deg_mode && has_rad) {
            snprintf(line, sizeof(line), "= %s (%s)", rad_angle, sym_ans);
            lcd_draw_text(14, 96, line, COL_ANS, COL_DISP_BG, 2);
        } else if (has_rad) {
            snprintf(line, sizeof(line), "RAD: %s", rad_angle);
            lcd_draw_text(14, 88, line, COL_ACCENT, COL_DISP_BG, 1);
            snprintf(line, sizeof(line), "= %s", sym_ans);
            lcd_draw_text(14, 102, line, COL_ANS, COL_DISP_BG, 2);
        } else {
            snprintf(line, sizeof(line), "= %s", sym_ans);
            lcd_draw_text(14, 96, line, COL_ANS, COL_DISP_BG, 2);
        }
    } else {
        lcd_draw_text(14, 96, "SYNTAX / UNDEF", COL_RED, COL_DISP_BG, 2);
    }
}
