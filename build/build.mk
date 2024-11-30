# note from pkhead:
# consider using an actual build system
# meson or cmake
# batch files/shell scripts and makefiles are difficult to use and to make platform/compiler-agnostic.

CXX ?= c++

# list of all cpp source files
SOURCES=$(wildcard src/*.cpp) $(APP_SOURCES) $(wildcard src/font/*.cpp) $(wildcard src/math/*.cpp)
CPPFLAGS += --std=c++17
INCLUDES = -I"include/"

# debug/release mode
ifeq ($(buildmode),debug)
  CPPFLAGS += -g
else ifeq ($(buildmode),release)
  CPPFLAGS += -O2
  ifeq ($(OS),Windows_NT)
    CPPFLAGS += -mwindows -static
  endif
endif

# determine libs to link with based off platform
ifeq ($(OS),Windows_NT)
  LIBS += lib/GLFW/libglfw3.a -lgdi32 -lopengl32 -luser32 -lcomdlg32 -lole32
else
  # items to plug into pkg-config to find libs and includes
  REQPKGS += glfw3 gtk+-3.0
  LIBS += -lGL
endif

# find libs and includes from REQPKGS list. works if empty.
INCLUDES += $(foreach pkg,$(REQPKGS),$(shell pkg-config --cflags $(pkg)))
LIBS += $(foreach pkg,$(REQPKGS),$(shell pkg-config --libs $(pkg)))

# collect list of all .o files associated with source .cpp files
OBJS = $(addprefix build/obj/, $(shell realpath --relative-to src $(SOURCES:%.cpp=%.o)))

# link all .o files to build FloodForge executable
build/FloodForge: pkgcheck $(OBJS)
	@echo [LNK] $@
	@$(CXX) -o $@ $(CPPFLAGS) $(OBJS) $(LIBS)

# instruct make on how build an .o file from a .cpp file
build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo [CXX] $<
	@$(CXX) -c $(CPPFLAGS) $(INCLUDES) $< -o $@

# generate dependencies from headers included in source files
# uses the c++ preprocessor to determine header dependencies
build/.depend: $(SOURCES)
	@echo [INF] Updating header dependency graph...
	@$(CXX) -MM $^ $(CPPFLAGS) $(INCLUDES) > $@ && \
	sed -Ei 's#^(.*\.o: *)src/(.*/)?(.*\.cpp)#build/obj/\2\1src/\2\3#' $@

# check if any required packages are missing. if so, stop compilation.
pkgcheck:
  ifneq ($(shell $(foreach pkg,$(REQPKGS),pkg-config --exists $(pkg) &&) echo 1),1)
  $(error Missing required packages: $(foreach pkg,$(REQPKGS),$(shell pkg-config --exists $(pkg) || echo $(pkg))))
  endif	

.PHONY: pkgcheck
include build/.depend