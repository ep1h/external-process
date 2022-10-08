# SETTINGS #####################################################################
EXTERNAL_PROCESS_SIMULATOR_NAME ?= external_process_simulator.exe
APP_NAME:=$(EXTERNAL_PROCESS_SIMULATOR_NAME)
OBJ_DIR:=obj
BIN_DIR:=bin
SRC_ROOT_DIR_S:=src

C_COMPILER:=i686-w64-mingw32-gcc
CPP_COMPILER:=i686-w64-mingw32-g++
CMP_FLAGS:=-Wall -Wextra -Werror -O2 -DTEST_RUN
LINKER:=$(CPP_COMPILER)
LINK_FLAGS:=-static #--disable-dynamicbase -Wl,--image-base,0x400000

# IMPLEMENTATION ###############################################################
build: $(OBJ_DIR) $(BIN_DIR) $(BIN_DIR)/$(APP_NAME)

rwc=$(sort $(wildcard $1$2)) $(foreach d,$(wildcard $1*),$(call rwc,$d/,$2))

SRC_ROOT_DIRS:=$(SRC_ROOT_DIR_S)
SRC_DIRS:=$(sort $(dir $(foreach d,$(SRC_ROOT_DIRS),$(call rwc,$(d),*/))))

VPATH := $(SRC_DIRS)

$(OBJ_DIR):
#	if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
#	if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
	mkdir -p $(BIN_DIR)

CPP_FILES:=$(notdir $(foreach d,$(SRC_ROOT_DIRS),$(call rwc,$(d),*.cpp)))
C_FILES:=$(notdir $(foreach d,$(SRC_ROOT_DIRS),$(call rwc,$(d),*.c)))
HPP_FILES:=$(foreach d,$(SRC_ROOT_DIRS),$(call rwc,$(d),*.hpp))
H_FILES:=$(foreach d,$(SRC_ROOT_DIRS),$(call rwc,$(d),*.h))

H_HPP_DIRS:=$(dir $(HPP_FILES)) $(dir $(H_FILES))
INCLUDES:=$(addprefix -I, $(H_HPP_DIRS))

OBJ_FILES := $(CPP_FILES:%=$(OBJ_DIR)/%.o) $(C_FILES:%=$(OBJ_DIR)/%.o)

# compile .cpp files
$(OBJ_DIR)/%.cpp.o: %.cpp
	$(CPP_COMPILER) -c -o $@ $(CMP_FLAGS) $(INCLUDES) $<

# compile .c files
$(OBJ_DIR)/%.c.o: %.c
	$(C_COMPILER) -c -o $@ $(CMP_FLAGS) $(INCLUDES) $<

# link
$(BIN_DIR)/$(APP_NAME): $(OBJ_FILES)
	$(LINKER) $(LINK_FLAGS) -o $@ $^

clean:
#	if exist $(BIN_DIR) rmdir /Q /S $(BIN_DIR)
#	if exist $(OBJ_DIR) rmdir /Q /S $(OBJ_DIR)
#	del /Q /F /S $(BIN_DIR) $(OBJ_DIR)
	rm -fr $(BIN_DIR)
	rm -fr $(OBJ_DIR)
