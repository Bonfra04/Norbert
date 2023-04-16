DIR_ROOT = .
DIR_BIN	= ./bin
DIR_OBJ	= ./bin-int

C_FILES = $(shell find . -wholename "./src/*.c")
OBJ_FILES = $(C_FILES:%.c=$(DIR_OBJ)/%.o)

CC_FLAGS = -g -O0 -c -I $(DIR_ROOT)/include -I $(DIR_ROOT)/src -Wno-address-of-packed-member

.PHONY: all clean

all: clean $(OBJ_FILES)
	@mkdir -p $(DIR_BIN)
	@echo "Linking"
	@gcc $(OBJ_FILES) -o $(DIR_BIN)/Norbert

clean:
	@rm -rf $(DIR_BIN) $(DIR_OBJ)

$(OBJ_FILES): $(DIR_OBJ)/%.o: %.c
	@echo "Compiling $(shell basename $<)"
	@mkdir -p $(shell dirname $@)
	@gcc $(CC_FLAGS) $< -o $@
