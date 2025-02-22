DIR_ROOT = .
DIR_BIN	= ./bin
DIR_OBJ	= ./bin-int

C_FILES = $(shell find . -wholename "./src/*.cpp")
OBJ_FILES = $(C_FILES:%.cpp=$(DIR_OBJ)/%.o)
DEP_FILES = $(OBJ_FILES:.o=.d)

CC_FLAGS = -MMD -MP -g -O0 -c -Wall -Wno-unused-label -I $(DIR_ROOT)/include -I $(DIR_ROOT)/src -Wno-address-of-packed-member -Wno-dangling-else

.PHONY: all clean

all: clean exec 
	
exec: $(OBJ_FILES)
	@mkdir -p $(DIR_BIN)
	@echo "Linking"
	@g++ $(OBJ_FILES) -o $(DIR_BIN)/Norbert

clean:
	@rm -rf $(DIR_BIN) $(DIR_OBJ)

$(OBJ_FILES): $(DIR_OBJ)/%.o: %.cpp
	@echo "Compiling $(shell basename $<)"
	@mkdir -p $(shell dirname $@)
	@g++ $(CC_FLAGS) $(TEST) $< -o $@

-include $(DEP_FILES)
