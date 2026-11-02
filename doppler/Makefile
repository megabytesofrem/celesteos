CC ?= cc
CXX ?= c++

BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/dopplerc

CFLAGS ?= -std=c17 -O2 -Wall -Wextra -Wpedantic
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

C_SOURCES := $(wildcard *.c)
CXX_SOURCES := $(wildcard *.cc *.cpp *.cxx)
C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.c.o,$(C_SOURCES))
CXX_OBJECTS := $(patsubst %.cc,$(BUILD_DIR)/%.cc.o,$(filter %.cc,$(CXX_SOURCES))) \
	$(patsubst %.cpp,$(BUILD_DIR)/%.cpp.o,$(filter %.cpp,$(CXX_SOURCES))) \
	$(patsubst %.cxx,$(BUILD_DIR)/%.cxx.o,$(filter %.cxx,$(CXX_SOURCES)))
OBJECTS := $(C_OBJECTS) $(CXX_OBJECTS)

.PHONY: all clean run test-ast

all: $(TARGET)

ifeq ($(strip $(OBJECTS)),)
$(TARGET):
	@echo "No C or C++ compiler sources found in doppler/." >&2
	@echo "Add compiler sources, then run 'make -C doppler'." >&2
	@exit 1
else
$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(if $(strip $(CXX_OBJECTS)),$(CXX),$(CC)) $(LDFLAGS) $^ $(LDLIBS) -o $@
endif

$(BUILD_DIR)/%.c.o: %.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.cc.o: %.cc Makefile
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp Makefile
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.cxx.o: %.cxx Makefile
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	@./$(TARGET)

test-ast: $(BUILD_DIR)/tests/ast_test
	@./$(BUILD_DIR)/tests/ast_test

$(BUILD_DIR)/tests/ast_test: tests/ast_test.c ast.c ast_base.h ast/ast_expression.h ast/ast_literal.h lexer.h error.h Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/ast_test.c ast.c $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)
