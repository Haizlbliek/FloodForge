CXX := g++
CXXFLAGS := -std=c++17 -Iinclude/
LDFLAGS := -Llib/GLFW/ -lglfw3 -lgdi32 -lopengl32 -luser32 -lcomdlg32 -lole32
DEBUG_FLAGS := -g
RELEASE_FLAGS := -static -O2

OBJ_DIR := build/obj
DEP_DIR := build/dep
BIN := FloodForge.exe

CORE_SRC := src/*.cpp src/glad.c src/font/*.cpp src/math/*.cpp src/popup/*.cpp
MODULE_SRC := $(wildcard src/$(MODULE)/*.cpp)

MODULE ?= world

SRC := $(wildcard $(CORE_SRC)) $(MODULE_SRC)
OBJ := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(filter %.cpp,$(SRC))) $(patsubst %.c,   $(OBJ_DIR)/%.o, $(filter %.c,$(SRC)))
DEP := $(OBJ:.o=.d)


all: debug

debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(BIN)

release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(BIN)

$(BIN): $(OBJ)
	@echo Linking $@
	@$(CXX) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	@echo Compiling $<
	@mkdir -p $(dir $@) $(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: %.c
	@echo Compiling $<
	@mkdir -p $(dir $@) $(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEP)

clean:
	@rm -rf $(OBJ_DIR) $(DEP_DIR) $(BIN)

.PHONY: all debug release clean

run: debug
	@echo Running $(BIN)...
	@./$(BIN)