
ifeq ($(OS)$(os), Windows_NT)
	CROSS_PREFIX := C:/tools/GccX86_64_4.9.4-p13/x86_64-linux-gnu/bin/x86_64-linux-gnu-
	CROSS_SUFFIX := .exe
	MKDIR_P := python ../../../Platform/scripts/Utils/kUtil.py mkdir_p
	RM_F := python ../../../Platform/scripts/Utils/kUtil.py rm_f
	RM_RF := python ../../../Platform/scripts/Utils/kUtil.py rm_rf
	CP := python ../../../Platform/scripts/Utils/kUtil.py cp
else
	ifneq ($(shell uname -m), x86_64)
		CROSS_PREFIX := /tools/GccX86_64_4.9.4-p13/x86_64-linux-gnu/bin/x86_64-linux-gnu-
		CROSS_SUFFIX := 
	endif
	MKDIR_P := mkdir -p
	RM_F := rm -f
	RM_RF := rm -rf
	CP := cp
endif

C_COMPILER := $(CROSS_PREFIX)gcc$(CROSS_SUFFIX)
CXX_COMPILER := $(CROSS_PREFIX)g++$(CROSS_SUFFIX)
LINKER := $(CROSS_PREFIX)g++$(CROSS_SUFFIX)
ARCHIVER := $(CROSS_PREFIX)ar$(CROSS_SUFFIX)
GNU_READELF := $(CROSS_PREFIX)readelf$(CROSS_SUFFIX)
APP_GEN := python ../../../Platform/scripts/Utils/kAppGen.py

ifndef verbose
	SILENT := @
endif

ifndef config
	config := Debug
endif

ifeq ($(config),Debug)
	TARGET := ../../bin/linux_x64d/GoSdkExample
	INTERMEDIATES := 
	OBJ_DIR := ../../build/GoSdkExample-gnumk_linux_x64-Debug
	PREBUILD := 
	POSTBUILD := 
	COMPILER_FLAGS := -g -fpic
	C_FLAGS := -std=gnu99 -Wall -Wno-unused-variable -Wno-unused-parameter -Wno-unused-value -Wno-missing-braces
	CXX_FLAGS := -std=c++14 -Wall -Wfloat-conversion
	INCLUDE_DIRS := -I../../Platform/kApi -I../../Gocator/GoSdk
	DEFINES :=
	LINKER_FLAGS := -Wl,-no-undefined -Wl,--allow-shlib-undefined -Wl,-rpath,'$$ORIGIN/../../lib/linux_x64d' -Wl,-rpath-link,../../lib/linux_x64d
	LIB_DIRS := -L../../lib/linux_x64d
	LIBS := -Wl,--start-group -lkApi -lGoSdk -Wl,--end-group
	LDFLAGS := $(LINKER_FLAGS) $(LIBS) $(LIB_DIRS)

	OBJECTS := ../../build/GoSdkExample-gnumk_linux_x64-Debug/GoSdkExample.o
	DEP_FILES = ../../build/GoSdkExample-gnumk_linux_x64-Debug/GoSdkExample.d
	TARGET_DEPS = ./../../lib/linux_x64d/libGoSdk.so

endif

ifeq ($(config),Release)
	TARGET := ../../bin/linux_x64/GoSdkExample
	INTERMEDIATES := 
	OBJ_DIR := ../../build/GoSdkExample-gnumk_linux_x64-Release
	PREBUILD := 
	POSTBUILD := 
	COMPILER_FLAGS := -O2 -fpic
	C_FLAGS := -std=gnu99 -Wall -Wno-unused-variable -Wno-unused-parameter -Wno-unused-value -Wno-missing-braces
	CXX_FLAGS := -std=c++14 -Wall -Wfloat-conversion
	INCLUDE_DIRS := -I../../Platform/kApi -I../../Gocator/GoSdk
	DEFINES :=
	LINKER_FLAGS := -Wl,-no-undefined -Wl,--allow-shlib-undefined -Wl,-rpath,'$$ORIGIN/../../lib/linux_x64' -Wl,-rpath-link,../../lib/linux_x64
	LIB_DIRS := -L../../lib/linux_x64
	LIBS := -Wl,--start-group -lkApi -lGoSdk -Wl,--end-group
	LDFLAGS := $(LINKER_FLAGS) $(LIBS) $(LIB_DIRS)

	OBJECTS := ../../build/GoSdkExample-gnumk_linux_x64-Release/GoSdkExample.o
	DEP_FILES = ../../build/GoSdkExample-gnumk_linux_x64-Release/GoSdkExample.d
	TARGET_DEPS = ./../../lib/linux_x64/libGoSdk.so

endif

.PHONY: all all-obj all-dep clean

all: $(OBJ_DIR)
	$(PREBUILD)
	$(SILENT) $(MAKE) -f GoSdkExample-Linux_X64.mk all-dep
	$(SILENT) $(MAKE) -f GoSdkExample-Linux_X64.mk all-obj

clean:
	$(SILENT) $(info Cleaning $(OBJ_DIR))
	$(SILENT) $(RM_RF) $(OBJ_DIR)
	$(SILENT) $(info Cleaning $(TARGET) $(INTERMEDIATES))
	$(SILENT) $(RM_F) $(TARGET) $(INTERMEDIATES)

all-obj: $(OBJ_DIR) $(TARGET)
all-dep: $(OBJ_DIR) $(DEP_FILES)

$(OBJ_DIR):
	$(SILENT) $(MKDIR_P) $@

ifeq ($(config),Debug)

$(TARGET): $(OBJECTS) $(TARGET_DEPS)
	$(SILENT) $(info LdX64 $(TARGET))
	$(SILENT) $(LINKER) $(OBJECTS) $(LDFLAGS) -o$(TARGET)

endif

ifeq ($(config),Release)

$(TARGET): $(OBJECTS) $(TARGET_DEPS)
	$(SILENT) $(info LdX64 $(TARGET))
	$(SILENT) $(LINKER) $(OBJECTS) $(LDFLAGS) -o$(TARGET)

endif

ifeq ($(config),Debug)

../../build/GoSdkExample-gnumk_linux_x64-Debug/GoSdkExample.o ../../build/GoSdkExample-gnumk_linux_x64-Debug/GoSdkExample.d: GoSdkExample/GoSdkExample.c
	$(SILENT) $(info GccX64 GoSdkExample/GoSdkExample.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdkExample-gnumk_linux_x64-Debug/GoSdkExample.o -c GoSdkExample/GoSdkExample.c -MMD -MP

endif

ifeq ($(config),Release)

../../build/GoSdkExample-gnumk_linux_x64-Release/GoSdkExample.o ../../build/GoSdkExample-gnumk_linux_x64-Release/GoSdkExample.d: GoSdkExample/GoSdkExample.c
	$(SILENT) $(info GccX64 GoSdkExample/GoSdkExample.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdkExample-gnumk_linux_x64-Release/GoSdkExample.o -c GoSdkExample/GoSdkExample.c -MMD -MP

endif

ifeq ($(MAKECMDGOALS),all-obj)

ifeq ($(config),Debug)

include ../../build/GoSdkExample-gnumk_linux_x64-Debug/GoSdkExample.d

endif

ifeq ($(config),Release)

include ../../build/GoSdkExample-gnumk_linux_x64-Release/GoSdkExample.d

endif

endif

