LIBNAME := ep_core.lib
CC32 := i686-w64-mingw32-gcc
CXX32 := i686-w64-mingw32-g++
CC64 := x86_64-w64-mingw32-gcc
CXX64 := x86_64-w64-mingw32-g++
CFLAGS = -O3 -Wall -Wextra -Werror
CXXFLAGS = -O3 -Wall -Wextra -Werror
CLINKFLAGS = -static
CXXLINKFLAGS = -static

ARCH ?= 32
ifeq ($(ARCH),64)
    CC := $(CC64)
    CXX := $(CXX64)
    OUTPUT_DIR = build/x64
else
    CC := $(CC32)
    CXX := $(CXX32)
    OUTPUT_DIR = build/x32
endif

rwc = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwc,$d/,$2))

CORE_C_SOURCES = $(call rwc,src/core/,*.c)
CORE_CXX_SOURCES = $(call rwc,src/core/,*.cpp)

CORE_C_OBJECTS = $(patsubst src/%, $(OUTPUT_DIR)/obj/src/%, $(CORE_C_SOURCES:.c=.o))
CORE_CXX_OBJECTS = $(patsubst src/%, $(OUTPUT_DIR)/obj/src/%, $(CORE_CXX_SOURCES:.cpp=.o))

help:
	@echo "Available targets:"
	@echo "  help:  Display this help message (default)"
	@echo "  build_core_lib: Build the core library."
	@echo "    Use ARCH=32 (default) for 32-bit or ARCH=64 for 64-bit."
	@echo "  clean: Remove all build artifacts."

$(OUTPUT_DIR)/obj/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_DIR)/obj/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT_DIR)/lib/$(LIBNAME): $(CORE_C_OBJECTS) $(CORE_CXX_OBJECTS)
	mkdir -p $(dir $@)
	ar rcs $@ $^

clean:
	rm -rf build

build_core_lib: $(OUTPUT_DIR)/lib/$(LIBNAME)

.PHONY: clean help build_core_lib
