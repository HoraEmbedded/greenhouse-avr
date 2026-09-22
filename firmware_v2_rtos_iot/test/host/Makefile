CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -Isrc/core -Iinclude
SRC_DIR := ../../src/core
TEST_DIR := .

TESTS   := $(wildcard $(TEST_DIR)/test_*.c)
TARGETS := $(TESTS:.c=)

CORE_SRCS := $(wildcard $(SRC_DIR)/*.c)

.PHONY: all run clean

all: $(TARGETS)

%: %.c $(CORE_SRCS)
	$(CC) $(CFLAGS) $< $(CORE_SRCS) -o $@

run: all
	@for t in $(TARGETS); do echo "== $$t =="; ./$$t || exit 1; done

clean:
	rm -f $(TARGETS)