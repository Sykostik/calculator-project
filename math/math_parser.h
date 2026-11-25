#ifndef MATH_PARSER_H
#define MATH_PARSER_H

#include <stdbool.h>
#include <stddef.h>

bool math_is_deg_mode(void);
void math_toggle_deg_mode(void);
void math_set_eval_x(bool enabled, double x_val);

bool math_parse_and_calculate(const char *str, int start, int end, double *out_res);
bool math_symbolic_diff(const char *str, int start, int end, char *out, size_t out_sz);
void math_evaluate_expression(const char *expr, int expr_len);

#endif /* MATH_PARSER_H */
