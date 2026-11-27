/*
 * test_main.c — unit tests for the ESP32-S3 AI calculator.
 *
 * Runs on a normal PC (not on the ESP32): the real project code
 * (math_parser.c, calc_ui.c, ai_overlay.c, chat_history.c, keypad.c)
 * is compiled together with stubs that replace the display, GPIO,
 * Wi-Fi, camera and Gemini (see host_stubs.c).
 *
 * Build and run:  make -C test        (or see test/README.md)
 * Exit code: 0 if all tests pass, 1 if any test fails.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "app_config.h"
#include "math_parser.h"
#include "calc_ui.h"
#include "ai_overlay.h"
#include "chat_history.h"
#include "keypad.h"
#include "driver/gpio.h"
#include "host_stubs.h"

/* ================= Minimal test framework ================= */

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond, name) do {                                          \
        tests_run++;                                                    \
        if (cond) {                                                     \
            printf("  [PASS] %s\n", name);                              \
        } else {                                                        \
            tests_failed++;                                             \
            printf("  [FAIL] %s  (%s:%d)\n", name, __FILE__, __LINE__); \
        }                                                               \
    } while (0)

#define CHECK_NEAR(actual, expected, eps, name) do {                     \
        double a_ = (actual), e_ = (expected);                          \
        tests_run++;                                                    \
        if (fabs(a_ - e_) <= (eps)) {                                   \
            printf("  [PASS] %s\n", name);                              \
        } else {                                                        \
            tests_failed++;                                             \
            printf("  [FAIL] %s: got %.10g, expected %.10g  (%s:%d)\n", \
                   name, a_, e_, __FILE__, __LINE__);                   \
        }                                                               \
    } while (0)

#define CHECK_STR(actual, expected, name) do {                          \
        const char *a_ = (actual), *e_ = (expected);                    \
        tests_run++;                                                    \
        if (a_ != NULL && strcmp(a_, e_) == 0) {                        \
            printf("  [PASS] %s\n", name);                              \
        } else {                                                        \
            tests_failed++;                                             \
            printf("  [FAIL] %s: got \"%s\", expected \"%s\"  (%s:%d)\n",\
                   name, a_ ? a_ : "(null)", e_, __FILE__, __LINE__);   \
        }                                                               \
    } while (0)

#define SECTION(title) printf("\n== %s ==\n", title)

/* ================= Helpers ================= */

/* Evaluate an expression with the real parser. Returns false on error. */
static bool calc(const char *expr, double *out) {
    *out = 0.0;
    return math_parse_and_calculate(expr, 0, (int)strlen(expr), out);
}

static double calc_value(const char *expr) {
    double v = 0.0;
    calc(expr, &v);
    return v;
}

static bool calc_fails(const char *expr) {
    double v;
    return !calc(expr, &v);
}

/* Symbolic derivative of expr. Returns the formula, or NULL if unsupported. */
static const char *diff(const char *expr) {
    static char out[64];
    if (!math_symbolic_diff(expr, 0, (int)strlen(expr), out, sizeof(out))) return NULL;
    return out;
}

/* Run "=SOLV" on expr and return the result line the calculator would show. */
static const char *solve_on_screen(const char *expr) {
    stub_lcd_clear_log();
    math_evaluate_expression(expr, (int)strlen(expr));
    const char *line = stub_lcd_find("= ");
    if (line == NULL) line = stub_lcd_find("SYNTAX");
    return line;
}

static void set_deg_mode(bool deg) {
    if (math_is_deg_mode() != deg) math_toggle_deg_mode();
}

/* Press a sequence of keys on the calculator, e.g. "12A1". */
static void press(const char *keys) {
    for (const char *k = keys; *k; k++) {
        calc_handle_key(*k);
    }
}

/* Back to the Numbers layer with an empty line and an empty clipboard.
 * A always leaves layer 0, then B always returns to layer 0;
 * the first C clears the line, the second clears the clipboard. */
static void calc_reset(void) {
    press("ABCC");
}

/* ================= Tests ================= */

static void test_arithmetic(void) {
    SECTION("Arithmetic and order of operations");
    CHECK_NEAR(calc_value("2+3*4"), 14, 1e-9, "2+3*4 = 14");
    CHECK_NEAR(calc_value("(2+3)*4"), 20, 1e-9, "(2+3)*4 = 20");
    CHECK_NEAR(calc_value("10-4-3"), 3, 1e-9, "10-4-3 = 3 (left to right)");
    CHECK_NEAR(calc_value("100/10/5"), 2, 1e-9, "100/10/5 = 2 (left to right)");
    CHECK_NEAR(calc_value("8/2*4"), 16, 1e-9, "8/2*4 = 16");
    CHECK_NEAR(calc_value("7%3"), 1, 1e-9, "7%3 = 1 (remainder)");
    CHECK_NEAR(calc_value("2^10"), 1024, 1e-9, "2^10 = 1024");
    CHECK_NEAR(calc_value("2^-1"), 0.5, 1e-9, "2^-1 = 0.5");
    CHECK_NEAR(calc_value("2^2*3"), 12, 1e-9, "2^2*3 = 12 (power before multiply)");
    CHECK_NEAR(calc_value("-3+5"), 2, 1e-9, "-3+5 = 2");
    CHECK_NEAR(calc_value("5--3"), 8, 1e-9, "5--3 = 8");
    CHECK_NEAR(calc_value("2*-3"), -6, 1e-9, "2*-3 = -6");
    CHECK_NEAR(calc_value("((1+2)*(3+4))"), 21, 1e-9, "nested parentheses");
    CHECK_NEAR(calc_value("1.5+.5"), 2, 1e-9, "decimals 1.5+.5 = 2");
    CHECK_NEAR(calc_value("|3-8|"), 5, 1e-9, "|3-8| = 5 (absolute value)");
}

static void test_functions(void) {
    SECTION("Functions and constants");
    CHECK_NEAR(calc_value("sqrt(16)"), 4, 1e-9, "sqrt(16) = 4");
    CHECK_NEAR(calc_value("cbrt(27)"), 3, 1e-9, "cbrt(27) = 3");
    CHECK_NEAR(calc_value("log(1000)"), 3, 1e-9, "log(1000) = 3");
    CHECK_NEAR(calc_value("log2(8)"), 3, 1e-9, "log2(8) = 3 (any base)");
    CHECK_NEAR(calc_value("ln(e)"), 1, 1e-9, "ln(e) = 1");
    CHECK_NEAR(calc_value("exp(0)"), 1, 1e-9, "exp(0) = 1");
    CHECK_NEAR(calc_value("fact(5)"), 120, 1e-9, "fact(5) = 120");
    CHECK_NEAR(calc_value("fact(0)"), 1, 1e-9, "fact(0) = 1");
    CHECK_NEAR(calc_value("fact(170)"), 7.257415615307994e306, 1e294, "fact(170) is the largest allowed");
    CHECK_NEAR(calc_value("pi"), M_PI, 1e-12, "pi");
    CHECK_NEAR(calc_value("e"), 2.718281828459045, 1e-12, "e");
    CHECK_NEAR(calc_value("2*pi"), 2 * M_PI, 1e-12, "2*pi");
}

static void test_trigonometry(void) {
    SECTION("Trigonometry: DEG and RAD");
    set_deg_mode(true);
    CHECK_NEAR(calc_value("sin(30)"), 0.5, 1e-9, "DEG: sin(30) = 0.5");
    CHECK_NEAR(calc_value("cos(60)"), 0.5, 1e-9, "DEG: cos(60) = 0.5");
    CHECK_NEAR(calc_value("tan(45)"), 1, 1e-9, "DEG: tan(45) = 1");
    CHECK_NEAR(calc_value("ctg(45)"), 1, 1e-9, "DEG: ctg(45) = 1");
    CHECK_NEAR(calc_value("asin(1)"), 90, 1e-9, "DEG: asin(1) = 90");
    CHECK_NEAR(calc_value("acos(0)"), 90, 1e-9, "DEG: acos(0) = 90");
    CHECK_NEAR(calc_value("atan(1)"), 45, 1e-9, "DEG: atan(1) = 45");
    CHECK(calc_fails("tan(90)"), "DEG: tan(90) is undefined");

    set_deg_mode(false);
    CHECK_NEAR(calc_value("sin(pi/2)"), 1, 1e-9, "RAD: sin(pi/2) = 1");
    CHECK_NEAR(calc_value("cos(pi)"), -1, 1e-9, "RAD: cos(pi) = -1");
    CHECK_NEAR(calc_value("asin(1)"), M_PI / 2, 1e-9, "RAD: asin(1) = pi/2");
    set_deg_mode(true);
}

static void test_errors(void) {
    SECTION("Invalid operations are rejected (SYNTAX / UNDEF)");
    CHECK(calc_fails("1/0"), "division by zero");
    CHECK(calc_fails("5%0"), "remainder by zero");
    CHECK(calc_fails("sqrt(-4)"), "sqrt of a negative number");
    CHECK(calc_fails("log(0)"), "log(0)");
    CHECK(calc_fails("ln(-1)"), "ln of a negative number");
    CHECK(calc_fails("log1(5)"), "logarithm with base 1");
    CHECK(calc_fails("fact(171)"), "fact(171) overflows double");
    CHECK(calc_fails("fact(2.5)"), "fact of a non-integer");
    CHECK(calc_fails("fact(-1)"), "fact of a negative number");
    CHECK(calc_fails("asin(2)"), "asin outside [-1, 1]");
    CHECK(calc_fails(""), "empty expression");
    CHECK(calc_fails("x+1"), "x is only allowed in graph mode");
}

static void test_variable_x(void) {
    SECTION("Variable x (graph mode) and numeric derivative");
    math_set_eval_x(true, 3.0);
    CHECK_NEAR(calc_value("x^2"), 9, 1e-9, "x=3: x^2 = 9");
    CHECK_NEAR(calc_value("2*x+1"), 7, 1e-9, "x=3: 2*x+1 = 7");
    CHECK_NEAR(calc_value("d/dx(x^2)"), 6, 1e-6, "x=3: d/dx(x^2) = 6 (numeric)");
    CHECK_NEAR(calc_value("d/dx(x*x*x)"), 27, 1e-5, "x=3: d/dx(x*x*x) = 27 (numeric works for products)");
    math_set_eval_x(false, 0.0);
    CHECK(calc_fails("x^2"), "x is rejected again after graph mode");
}

static void test_screen_output(void) {
    SECTION("What =SOLV shows on the screen");
    set_deg_mode(true);
    CHECK_STR(solve_on_screen("2+2"), "= 4", "2+2");
    CHECK_STR(solve_on_screen("1/2"), "= 1/2", "1/2 shown as a fraction");
    CHECK_STR(solve_on_screen("sin(60)"), "= sqrt(3)/2", "sin(60) shown exactly");
    CHECK_STR(stub_lcd_find("RAD:"), "RAD: PI/3", "sin(60): angle also shown as PI/3");
    CHECK_STR(solve_on_screen("cos(45)"), "= sqrt(2)/2", "cos(45) shown exactly");
    CHECK_STR(solve_on_screen("tan(60)"), "= sqrt(3)", "tan(60) shown exactly");
    CHECK_STR(solve_on_screen("fact(10)"), "= 3628800", "fact(10)");
    CHECK_STR(solve_on_screen("log2(8)"), "= 3", "log2(8)");
    CHECK_STR(solve_on_screen("1/0"), "SYNTAX / UNDEF", "1/0 shows an error");
    CHECK_STR(solve_on_screen("d/dx(x^3-2*x)"), "= 3*x^2-2", "=SOLV on d/dx gives a formula");
}

static void test_symbolic_derivative(void) {
    SECTION("Symbolic derivative: supported rules");
    CHECK_STR(diff("5"), "0", "(5)' = 0");
    CHECK_STR(diff("x"), "1", "(x)' = 1");
    CHECK_STR(diff("x^2"), "2*x", "(x^2)' = 2*x");
    CHECK_STR(diff("x^3"), "3*x^2", "(x^3)' = 3*x^2");
    CHECK_STR(diff("3*x"), "3", "(3*x)' = 3");
    CHECK_STR(diff("4*x^3"), "12*x^2", "(4*x^3)' = 12*x^2");
    CHECK_STR(diff("x^3-2*x"), "3*x^2-2", "(x^3-2*x)' = 3*x^2-2");
    CHECK_STR(diff("x^2+5"), "2*x", "(x^2+5)' = 2*x");
    CHECK_STR(diff("-x^2"), "-2*x", "(-x^2)' = -2*x");
    CHECK_STR(diff("sin(x)"), "cos(x)", "(sin x)' = cos x");
    CHECK_STR(diff("cos(x)"), "-sin(x)", "(cos x)' = -sin x");
    CHECK_STR(diff("tan(x)"), "1/cos(x)^2", "(tan x)' = 1/cos(x)^2");
    CHECK_STR(diff("ln(x)"), "1/x", "(ln x)' = 1/x");
    CHECK_STR(diff("exp(x)"), "exp(x)", "(exp x)' = exp x");
    CHECK_STR(diff("sqrt(x)"), "1/(2*sqrt(x))", "(sqrt x)' = 1/(2*sqrt(x))");
    CHECK_STR(diff("3*sin(x)"), "3*cos(x)", "(3*sin x)' = 3*cos x");

    SECTION("Symbolic derivative: unsupported input returns an error, not a wrong answer");
    CHECK(diff("x*sin(x)") == NULL, "product rule not supported yet");
    CHECK(diff("1/x") == NULL, "quotient not supported yet");
    CHECK(diff("sin(2*x)") == NULL, "chain rule not supported yet");
}

static void test_keypad_matrix(void) {
    SECTION("Keypad: 4x4 matrix scan (simulated GPIO)");
    const char expected[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    keypad_init();
    sim_keypad_release();
    CHECK(keypad_scan() == 0, "no key pressed -> 0");

    bool all_ok = true;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            sim_keypad_press(r, c);
            if (keypad_scan() != expected[r][c]) all_ok = false;
            sim_keypad_release();
            keypad_scan();
        }
    }
    CHECK(all_ok, "all 16 keys map to the right characters");

    sim_keypad_press(1, 1);
    char first = keypad_scan();
    char held = keypad_scan();
    CHECK(first == '5' && held == 0, "holding a key gives one press, not repeats");
    sim_keypad_release();
    keypad_scan();
    sim_keypad_press(1, 1);
    CHECK(keypad_scan() == '5', "pressing again after release works");
    sim_keypad_release();
    keypad_scan();
}

static void test_calculator_input(void) {
    SECTION("Calculator input: layers, cursor, editing");
    set_deg_mode(true);

    calc_reset();
    press("12");
    CHECK_STR(calc_get_expression(), "12", "Numbers layer types digits");

    press("A1");
    CHECK_STR(calc_get_expression(), "12+", "Layer A: key 1 is +");

    press("8");
    CHECK_STR(calc_get_expression(), "12+sqrt()", "Layer A: key 8 inserts sqrt()");
    press("B9");
    CHECK_STR(calc_get_expression(), "12+sqrt(9)", "cursor was placed inside the parentheses");

    stub_lcd_clear_log();
    press("A0");
    CHECK_STR(stub_lcd_find("= "), "= 15", "Layer A: key 0 (=SOLV) evaluates 12+sqrt(9)");

    calc_reset();
    press("123*D");
    CHECK_STR(calc_get_expression(), "13", "* moves cursor left, D deletes the char before it");
    press("#4");
    CHECK_STR(calc_get_expression(), "134", "# moves cursor right");
    press("C");
    CHECK_STR(calc_get_expression(), "", "C clears the line");

    calc_reset();
    press("AA1B30");
    CHECK_STR(calc_get_expression(), "sin(30)", "Layer B: key 1 inserts sin()");
    stub_lcd_clear_log();
    press("A0");
    CHECK_STR(stub_lcd_find("= "), "= 1/2", "sin(30) = 1/2 through the keypad");

    calc_reset();
    press("AAA4");
    CHECK_STR(calc_get_expression(), "pi", "Layer C: key 4 is pi");

    SECTION("Calculator input: Ans");
    calc_reset();
    press("6A3B7A0");             /* 6*7 =SOLV */
    calc_reset();
    press("AAA9");                /* Layer C: Ans */
    press("A");                   /* -> Layer D */
    press("A1");                  /* -> Layer A: + */
    press("B1");                  /* Numbers: 1 */
    stub_lcd_clear_log();
    press("A0");
    CHECK_STR(calc_get_expression(), "Ans+1", "expression is Ans+1");
    CHECK_STR(stub_lcd_find("= "), "= 43", "Ans keeps the last result (42)");

    SECTION("Calculator input: T9 letters");
    calc_reset();
    g_fake_time_us = 10 * 1000 * 1000;
    press("B");                   /* Numbers -> Letters */
    press("2");
    CHECK_STR(calc_get_expression(), "d", "key 2 once -> d");
    g_fake_time_us += 300 * 1000;
    press("2");
    g_fake_time_us += 300 * 1000;
    press("2");
    CHECK_STR(calc_get_expression(), "f", "key 2 three times within 800 ms -> f");
    g_fake_time_us += 1000 * 1000;
    press("2");
    CHECK_STR(calc_get_expression(), "fd", "after 800 ms the same key starts a new letter");
    press("0");
    CHECK_STR(calc_get_expression(), "fd ", "key 0 in Letters is a space");

    SECTION("Calculator input: clipboard (Layer D, key 0)");
    calc_reset();
    press("25AAAA0");             /* type 25, go to Layer D, Copy */
    CHECK(stub_lcd_find("Copied!") != NULL, "Copy shows Copied!");
    press("C0");                  /* clear line, Paste */
    CHECK_STR(calc_get_expression(), "25", "Paste inserts the copied text");
    press("CC");
    CHECK(stub_lcd_find("Clipboard Cleared") != NULL, "C on an empty line clears the clipboard");

    SECTION("Calculator input: Layer D actions are routed");
    calc_reset();
    int g0 = stub_calls_gemini, c0 = stub_calls_camera, gr0 = stub_calls_graph;
    press("AAAA1");
    CHECK(stub_calls_gemini == g0 + 1, "Layer D key 1 calls Gemini (Solv)");
    press("3");
    CHECK(stub_calls_camera == c0 + 1, "Layer D key 3 starts the camera");
    press("6");
    CHECK(stub_calls_graph == gr0 + 1, "Layer D key 6 opens graph mode");
}

static void test_chat_history(void) {
    SECTION("AI chat history (last 8 messages)");
    chat_init();
    CHECK(chat_get_count() == 0, "empty after init");

    chat_add(false, "q1");
    chat_add(true, "a1");
    CHECK(chat_get_count() == 2, "two messages stored");
    CHECK_STR(chat_get_msg(0)->text, "q1", "first message is the question");
    CHECK(chat_get_msg(1)->is_model, "second message is from the model");
    CHECK(chat_get_msg(5) == NULL, "out-of-range index returns NULL");

    char buf[8];
    for (int i = 2; i <= MAX_CHAT_TURNS; i++) {
        snprintf(buf, sizeof(buf), "m%d", i);
        chat_add(i % 2 == 0, buf);
    }
    CHECK(chat_get_count() == MAX_CHAT_TURNS - 1, "when full, the oldest question+answer pair is dropped");
    CHECK_STR(chat_get_msg(0)->text, "m2", "oldest kept message is now m2");

    static char long_text[MAX_CHAT_TEXT * 2];
    memset(long_text, 'a', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = '\0';
    chat_clear();
    chat_add(true, long_text);
    CHECK(strlen(chat_get_msg(0)->text) == MAX_CHAT_TEXT - 1, "long replies are safely cut to the buffer size");

    chat_clear();
    CHECK(chat_get_count() == 0, "NewC clears the history");
}

static void test_ai_overlay(void) {
    SECTION("AI answer window: select, copy and paste into the calculator");
    calc_reset();

    ai_overlay_show("TEST", "Result is 42 units");
    CHECK(ai_overlay_is_active(), "answer window opens");
    for (int i = 0; i < 10; i++) ai_overlay_handle_key('6');  /* cursor to the '4' */
    ai_overlay_handle_key('5');                                /* start selection */
    ai_overlay_handle_key('6');                                /* extend to the '2' */
    ai_overlay_handle_key('5');                                /* lock selection */
    ai_overlay_handle_key('0');                                /* copy */
    ai_overlay_handle_key('C');                                /* close */
    CHECK(!ai_overlay_is_active(), "C closes the answer window");

    press("AAAA0");                                            /* Layer D: Paste */
    CHECK_STR(calc_get_expression(), "42", "selected '42' is pasted into the expression");

    calc_reset();
    ai_overlay_show("TEST", "first line\nx^2+1");
    ai_overlay_handle_key('8');                                /* move to line 2 */
    ai_overlay_handle_key('0');                                /* copy whole line */
    ai_overlay_handle_key('C');
    press("AAAA0");
    CHECK_STR(calc_get_expression(), "x^2+1", "without selection, 0 copies the whole line");

    calc_reset();
    char many[256] = "";
    for (int i = 1; i <= 25; i++) {
        char line[8];
        snprintf(line, sizeof(line), "%d\n", i);
        strcat(many, line);
    }
    ai_overlay_show("TEST", many);
    ai_overlay_handle_key('#');                                /* page 2 */
    ai_overlay_handle_key('#');                                /* page 3 */
    ai_overlay_handle_key('0');
    ai_overlay_handle_key('C');
    press("AAAA0");
    CHECK_STR(calc_get_expression(), "21", "# turns pages by 10 lines (page 3 starts at line 21)");

    calc_reset();
    char long_word[100];
    memset(long_word, 'z', 99);
    long_word[99] = '\0';
    ai_overlay_show("TEST", long_word);
    ai_overlay_handle_key('0');
    ai_overlay_handle_key('C');
    press("AAAA0");
    CHECK((int)strlen(calc_get_expression()) == MAX_LINE_CHARS,
          "a word longer than the screen is split into lines of 48 chars");
    calc_reset();
}

#ifdef CHECK_KNOWN_BUGS
/*
 * Known issues found in code review. These tests describe the CORRECT
 * behaviour and FAIL on the current code. Build with:  make -C test known
 * They should pass once the fixes are made.
 */
static void test_known_bugs(void) {
    SECTION("KNOWN BUGS (expected to fail until fixed)");
    set_deg_mode(true);
    CHECK_NEAR(calc_value("-2^2"), -4, 1e-9, "-2^2 = -4 (unary minus after power)");
    CHECK_NEAR(calc_value("2^3^2"), 512, 1e-9, "2^3^2 = 512 (power is right-associative)");
    CHECK_NEAR(calc_value("e-1"), M_E - 1, 1e-9, "e-1 = 1.718...");
    CHECK(calc_fails("(2+3"), "unbalanced parenthesis is an error");
    CHECK(calc_fails("2x"), "2x without * is an error, not 2");
    CHECK_STR(diff("x^2*3"), "6*x", "(x^2*3)' = 6*x");
    CHECK_STR(diff("x^2/2"), "x", "(x^2/2)' = x");
    set_deg_mode(false);
    CHECK_STR(solve_on_screen("45*2"), "= 90", "RAD: 45*2 is just 90, not PI/2");
    CHECK_NEAR(calc_value("sin(30\x7f)"), 0.5, 1e-9, "RAD: sin(30 deg) = 0.5");
    set_deg_mode(true);
    CHECK_STR(solve_on_screen("1/20000"), "= 5e-05", "small numbers are not shown as 0");
}
#endif

int main(void) {
    printf("ESP32-S3 AI calculator - unit tests (host build)\n");

    test_arithmetic();
    test_functions();
    test_trigonometry();
    test_errors();
    test_variable_x();
    test_screen_output();
    test_symbolic_derivative();
    test_keypad_matrix();
    test_calculator_input();
    test_chat_history();
    test_ai_overlay();
#ifdef CHECK_KNOWN_BUGS
    test_known_bugs();
#endif

    printf("\n%d tests, %d passed, %d failed\n", tests_run, tests_run - tests_failed, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
