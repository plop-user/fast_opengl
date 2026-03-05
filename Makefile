# =========================
# Compilers & target
# =========================
CXX     := clang++
CC      := clang
TARGET  := test

# =========================
# Directories
# =========================
SRC_DIR := src
INC_DIR := headers
OBJ_DIR := build

# =========================
# Source files
# =========================
CPP_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
C_SRCS   := $(SRC_DIR)/glad.c

CPP_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CPP_SRCS))
C_OBJS   := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SRCS))

OBJS := $(CPP_OBJS) $(C_OBJS)

# =========================
# Flags
# =========================
CXXFLAGS := -std=c++23 -Wall -Wextra -g \
            -I$(INC_DIR) \
            -I$(INC_DIR)/glad \
            -I$(INC_DIR)/KHR  \
	    -I$(INC_DIR)/tinyobj

CFLAGS   := -Wall -Wextra -g \
            -I$(INC_DIR) \
            -I$(INC_DIR)/glad \
            -I$(INC_DIR)/KHR  \
	    -I$(INC_DIR)/tinyobj

LDLIBS := \
    -lSDL2 \
    -lSDL2_image \
    -lSDL2_ttf \
    -lGL \
    -lm \
    -lpthread \
    -ldl \
    -lrt \
    -lX11

# =========================
# Rules
# =========================
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all run clean

