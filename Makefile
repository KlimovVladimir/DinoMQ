CC       := gcc
TARGET   := dinomq

SRC_DIRS := src net mqtt
BUILD_DIR := build

CPPFLAGS := $(addprefix -I,$(SRC_DIRS))
CFLAGS   := -std=gnu11 -Wall -Wextra -Wpedantic -g -O0 -fsanitize=address,undefined
LDFLAGS  := -fsanitize=address,undefined
LDLIBS   :=

SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all run debug clean rebuild print

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS := -std=gnu11 -Wall -Wextra -Wpedantic -g -O0
debug: clean $(TARGET)

rebuild: clean all

print:
	@echo "SRCS=$(SRCS)"
	@echo "OBJS=$(OBJS)"

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)
