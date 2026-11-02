CC ?= cc
CXX ?= c++

BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/dopplerc

CFLAGS ?= -std=c17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=
LDLIBS ?=

C_SOURCES := $(wildcard *.c ast/*.c)
C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.c.o,$(C_SOURCES))
OBJECTS := $(C_OBJECTS)
DEPS := $(OBJECTS:.o=.d)
AST_HEADERS := $(wildcard ast/*.h) lexer.h error.h

.PHONY: all clean run test-ast

all: $(TARGET)

$(BUILD_DIR)/ast/ast_nodes.c.o: $(AST_HEADERS)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.c.o: %.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	@./$(TARGET)

test-ast: $(BUILD_DIR)/tests/ast_test
	@./$(BUILD_DIR)/tests/ast_test

$(BUILD_DIR)/tests/ast_test: tests/ast_test.c ast/ast_nodes.c $(AST_HEADERS) Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tests/ast_test.c ast/ast_nodes.c $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
