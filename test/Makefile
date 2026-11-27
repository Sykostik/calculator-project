# Host unit tests for the ESP32-S3 AI calculator.
# Run from the project folder:   make -C test          (all tests must pass)
#                                make -C test known    (also runs known-bug tests)

CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -g
ROOT    = ..
INCS    = -Istubs -I. -I$(ROOT) -I$(ROOT)/config -I$(ROOT)/drivers -I$(ROOT)/math \
          -I$(ROOT)/ai -I$(ROOT)/ui -I$(ROOT)/network

SRCS    = test_main.c host_stubs.c \
          $(ROOT)/math/math_parser.c \
          $(ROOT)/ui/calc_ui.c \
          $(ROOT)/ui/ai_overlay.c \
          $(ROOT)/ai/chat_history.c \
          $(ROOT)/drivers/keypad.c

.PHONY: test known clean

test: run_tests
	./run_tests

known: run_tests_known
	./run_tests_known

run_tests: $(SRCS)
	$(CC) $(CFLAGS) $(INCS) $(SRCS) -lm -o $@

run_tests_known: $(SRCS)
	$(CC) $(CFLAGS) -DCHECK_KNOWN_BUGS $(INCS) $(SRCS) -lm -o $@

clean:
	rm -f run_tests run_tests_known run_tests.exe run_tests_known.exe
