TARGET_EXEC ?= doonengine

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CFLAGS := -g -O3

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_DIRS += ./dependencies/include
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

LD_FLAGS := -lm -lglfw -lGL

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ -march=native $(LD_FLAGS)

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@ -march=native

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
