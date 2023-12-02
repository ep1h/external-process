LIBNAME := ep_core.lib
CC32 := i686-w64-mingw32-gcc
CXX32 := i686-w64-mingw32-g++
CC64 := x86_64-w64-mingw32-gcc
CXX64 := x86_64-w64-mingw32-g++
CFLAGS_RELEASE = -O3 -Wall -Wextra -Werror
CFLAGS_DEBUG = -g -O0
CXXFLAGS_RELEASE = -O3 -Wall -Wextra -Werror
CXXFLAGS_DEBUG = -g -O0
CLINKFLAGS = -static -static-libgcc
CXXLINKFLAGS = -static -static-libstdc++

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

CFG ?= release
ifeq ($(CFG),debug)
	CFLAGS = $(CFLAGS_DEBUG)
	CXXFLAGS = $(CXXFLAGS_DEBUG)
	OUTPUT_DIR := $(OUTPUT_DIR)/$(CFG)
else ifeq ($(CFG),release)
	CFLAGS = $(CFLAGS_RELEASE)
	CXXFLAGS = $(CXXFLAGS_RELEASE)
	OUTPUT_DIR := $(OUTPUT_DIR)/$(CFG)
else
	$(error Invalid value for CFG: $(CFG). Valid values are 'debug' and 'release'. Use help target for more information.)
endif

rwc = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwc,$d/,$2))

C_SOURCES = $(call rwc,src/,*.c)
CXX_SOURCES = $(call rwc,src/,*.cpp)

C_OBJECTS = $(patsubst src/%, $(OUTPUT_DIR)/obj/src/%, $(C_SOURCES:.c=.o))
CXX_OBJECTS = $(patsubst src/%, $(OUTPUT_DIR)/obj/src/%, $(CXX_SOURCES:.cpp=.o))

help:
	@echo "Available targets:"
	@echo "  help:  Display this help message (default)"
	@echo "  build: Build everything."
	@echo "    Use ARCH=32 (default) for 32-bit or ARCH=64 for 64-bit."
	@echo "    Use CFG=release (default) for release build or CFG=debug for debug build."
	@echo "  build_core_lib: Build the library."
	@echo "    Use ARCH=32 (default) for 32-bit or ARCH=64 for 64-bit."
	@echo "    Use CFG=release (default) for release build or CFG=debug for debug build."
	@echo "  build_sim: Build external process simulator (for tests)."
	@echo "    Use ARCH=32 (default) for 32-bit or ARCH=64 for 64-bit."
	@echo "    Use CFG=release (default) for release build or CFG=debug for debug build."
	@echo "  build_tests: Build unit tests."
	@echo "    Use ARCH=32 (default) for 32-bit or ARCH=64 for 64-bit."
	@echo "    Use CFG=release (default) for release build or CFG=debug for debug build."
	@echo "  clean: Remove all build artifacts."

$(OUTPUT_DIR)/obj/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_DIR)/obj/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT_DIR)/lib/$(LIBNAME): $(C_OBJECTS) $(CXX_OBJECTS)
	mkdir -p $(dir $@)
	ar rcs $@ $^

build_core_lib: $(OUTPUT_DIR)/lib/$(LIBNAME)
clean_core_lib:
	rm -rf build
build_tests:
	$(MAKE) -C test/tests build_tests ARCH=$(ARCH)
clean_tests:
	$(MAKE) -C test/tests clean
build_sim:
	$(MAKE) -C test/external_process_simulator build_external_process_simulator ARCH=$(ARCH)
clean_sim:
	$(MAKE) -C test/external_process_simulator clean
run_tests:
	$(MAKE) -C test run_tests

build: build_core_lib build_sim build_tests
clean: clean_core_lib clean_sim clean_tests

.PHONY: clean help build test
