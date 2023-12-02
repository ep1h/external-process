APPNAME := external_process_simulator.exe
CXX32 := i686-w64-mingw32-g++
CXX64 := x86_64-w64-mingw32-g++
CXXFLAGS_RELEASE = -O3 -Wall -Wextra -Werror
CXXFLAGS_DEBUG = -g -O0
CXXLINKFLAGS = -static-libstdc++ -static-libgcc -static

rwc = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwc,$d/,$2))

ARCH ?= 32
ifeq ($(ARCH),64)
    CXX := $(CXX64)
    BUILD_DIR = build/x64
else
    CXX := $(CXX32)
    BUILD_DIR = build/x32
endif

CFG ?= release
ifeq ($(CFG),debug)
	CXXFLAGS = $(CXXFLAGS_DEBUG)
	BUILD_DIR := $(BUILD_DIR)/$(CFG)
else ifeq ($(CFG),release)
	CXXFLAGS = $(CXXFLAGS_RELEASE)
	BUILD_DIR := $(BUILD_DIR)/$(CFG)
else
	$(error Invalid value for CFG: $(CFG). Valid values are 'debug' and 'release'. Use help target for more information.)
endif

CXX_SOURCES = $(call rwc,src/,*.cpp)
CXX_OBJECTS = $(patsubst src/%, $(BUILD_DIR)/obj/src/%, $(CXX_SOURCES:.cpp=.o))

help:
	@echo "Available targets:"
	@echo "  help:  Display this help message (default)"
	@echo "  build_external_process_simulator: Build external simulator."
	@echo "    Use ARCH=32 (default) for 32-bit or ARCH=64 for 64-bit."
	@echo "    Use CFG=release (default) for release build or CFG=debug for debug build."
	@echo "  clean: Remove all build artifacts."

$(BUILD_DIR)/obj/src/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/bin/$(APPNAME): $(CXX_OBJECTS)
	mkdir -p $(dir $@)
	$(CXX) $(CXX_OBJECTS) $(CXXLINKFLAGS) -o $@

build_external_process_simulator: $(BUILD_DIR)/bin/$(APPNAME)

clean:
	rm -rf build

.PHONY: clean help build_external_process_simulator
