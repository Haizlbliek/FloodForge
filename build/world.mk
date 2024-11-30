# note from pkhead:
# consider using an actual build system
# meson or cmake
# batch files/shell scripts and makefiles are difficult to use and to make platform/compiler-agnostic.
#
# TODO: couldn't figure out how to use static glfw from the libs folder. (usually, i just include glfw source as a submodule)
#		so right now, it just uses glfw from the package manager on Linux.

CXX ?= c++

# list of all cpp source files
SOURCES=$(wildcard src/font/*.cpp) $(wildcard src/math/*.cpp) $(wildcard src/world/*.cpp) $(wildcard src/*.cpp)
CPPFLAGS += --std=c++17
INCLUDES = -I"include/"

ifeq ($(OS),Windows_NT)
LIBS += lib/GLFW/libglfw3.a -lgdi32 -lopengl32 -luser32 -lcomdlg32 -lole32
else
# items to plug into pkg-config to find libs and includes
REQPKGS += glfw3 gtk+-3.0
endif

# find libs and includes from REQPKGS list. works if empty.
INCLUDES += $(foreach pkg,$(REQPKGS),$(shell pkg-config --cflags $(pkg)))
LIBS += $(foreach pkg,$(REQPKGS),$(shell pkg-config --libs $(pkg)))

# collect list of all .o files associated with source .cpp files
OBJS = $(addprefix build/obj/, $(shell realpath --relative-to src $(SOURCES:%.cpp=%.o)))

# build an .o file from a .cpp file
build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo [CXX] $<
	@$(CXX) -c $(CPPFLAGS) $(INCLUDES) $< -o $@

# link all .o files to build FloodForge executable
build/FloodForge: pkgcheck $(OBJS)
	@echo [LNK] $@
	@$(CXX) -o $@ $(OBJS) $(LIBS)

# throw error if any required packages are missing
pkgcheck:
ifneq ($(shell $(foreach pkg,$(REQPKGS),pkg-config --exists $(pkg) &&) echo 1),1)
$(error Missing required packages: $(foreach pkg,$(REQPKGS),$(shell pkg-config --exists $(pkg) || echo $(pkg))))
endif	

.PHONY: clean pkgcheck
clean:
	rm -rf build/obj