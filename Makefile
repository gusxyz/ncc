CC = cc

SRC_DIR = src
BUILD_DIR = build

SRC_DIRS = $(shell find $(SRC_DIR) -type d)
SRCS = $(shell find $(SRC_DIR) -type f -name '*.c')

CPPFLAGS = -Isrc
CFLAGS = -g -Wall -Wextra -MMD -MP

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

TARGET = $(BUILD_DIR)/ncc
TEST_SUITES = $(basename $(notdir $(wildcard tests/cases/*.sh)))
TEST_TARGETS = $(addprefix test-,$(TEST_SUITES))
TEST ?=

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^


$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TARGET)
	./tests/test.sh $(TEST)

$(TEST_TARGETS): test-%: $(TARGET)
	./tests/test.sh $*

test-list:
	@./tests/test.sh --list

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)

.PHONY: all test test-list $(TEST_TARGETS) clean
