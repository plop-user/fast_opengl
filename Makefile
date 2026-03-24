# =========================
# Compilers & target
# =========================
CXX     := clang++
CC      := clang
TARGET  := game

# =========================
# OS detection
# =========================
UNAME_S := $(shell uname -s)

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
            -I$(INC_DIR)/KHR \
            -I$(INC_DIR)/tinyobj

CFLAGS := -Wall -Wextra -g \
          -I$(INC_DIR) \
          -I$(INC_DIR)/glad \
          -I$(INC_DIR)/KHR \
          -I$(INC_DIR)/tinyobj

LDFLAGS :=
LDLIBS  :=

# =========================
# Platform-specific config
# =========================
ifeq ($(UNAME_S),Darwin)
    # macOS (Homebrew)
    CXXFLAGS += -I/opt/homebrew/include
    CFLAGS   += -I/opt/homebrew/include
    LDFLAGS  += -L/opt/homebrew/lib

    LDLIBS += -lSDL2 -lSDL2_image -lSDL2_ttf \
              -framework OpenGL \
              -framework Cocoa \
              -framework IOKit \
              -framework CoreVideo \
              -lm -lpthread

else
    # Linux
    LDLIBS += -lSDL2 -lSDL2_image -lSDL2_ttf \
              -lGL -lm -lpthread -ldl -lrt -lX11
endif

# =========================
# Rules
# =========================
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LDLIBS)

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
