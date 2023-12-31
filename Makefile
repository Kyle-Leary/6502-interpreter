# the local install of lib6502 for the interpreter.
CPULIB_PATH := lib6502
STATIC_CPULIB_PATH := $(CPULIB_PATH)/lib6502.a 

CC := gcc
# also include the api headers for the 6502 library we're using here.
CFLAGS += -Isrc -Wall -I$(CPULIB_PATH)/src/api -lncurses -g 

BACKEND_DIR := linux

# Directories
SRC_DIR := src 
OBJ_DIR := build

TARGET := asm

# just compile everything in SRC_DIR.
SRCS := $(shell find $(SRC_DIR) -type f -name "*.c") 

# Object files (corresponding .o files in the obj/ directory)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS)) 

# set this flag only when we run the "make test" rule.
ifdef TESTING
CFLAGS += "-DTESTING=1"
endif

# set this flag only when we run the "make test" rule.
ifdef VIEWER
CFLAGS += "-DVIEWER=1"
endif

all: clean $(TARGET)

# just link statically like another object file, since static libraries are basically just that.
$(TARGET): $(OBJS) $(STATIC_CPULIB_PATH)
	$(CC) -o $@ $^ $(STATIC_CPULIB_PATH) $(INCLUDES) $(CFLAGS) 

# Pattern rule to compile .c files into .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< -g

# make the 6502 library in the submodule.
$(STATIC_CPULIB_PATH):
	make -C lib6502

clean:
	rm -f $(shell find $(OBJ_DIR) -name "*.o") $(TARGET) $(STATIC_CPULIB_PATH)
