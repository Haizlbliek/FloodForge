# note from pkhead:
# consider using an actual build system
# meson or cmake
# batch files/shell scripts and makefiles are difficult to use and to make platform/compiler-agnostic.
#
# TODO: couldn't figure out how to use static glfw from the libs folder. (usually, i just include glfw source as a submodule)
#		so right now, it just uses glfw from the package manager.

CXX=c++

# list of all cpp source files
SOURCES=$(wildcard src/font/*.cpp) $(wildcard src/math/*.cpp) $(wildcard src/world/*.cpp) $(wildcard src/*.cpp)

CPPFLAGS=--std=c++17
INCLUDES=-I"include/" $(shell pkg-config --cflags glfw3) $(shell pkg-config --cflags gtk+-3.0)
LIBS=$(shell pkg-config --static --libs glfw3) $(shell pkg-config --libs gtk+-3.0) -lGL

# list of all .o files associated with source .cpp files
OBJS=$(addprefix build/obj/, $(shell realpath --relative-to src $(SOURCES:%.cpp=%.o)))

# build an .o file from a .cpp file
build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo [CXX] $<
	@$(CXX) -c $(CPPFLAGS) $(INCLUDES) $< -o $@

# link all .o files to build FloodForge executable
build/FloodForge: $(OBJS)
	@echo [LNK] $@
	@$(CXX) -o $@ $(OBJS) $(LIBS)

.PHONY: clean
clean:
	rm -rf build/obj