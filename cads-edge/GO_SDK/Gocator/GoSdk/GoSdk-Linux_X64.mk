
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
	TARGET := ../../lib/linux_x64d/libGoSdk.so
	INTERMEDIATES := 
	OBJ_DIR := ../../build/GoSdk-gnumk_linux_x64-Debug
	PREBUILD := 
	POSTBUILD := 
	COMPILER_FLAGS := -g -fpic
	C_FLAGS := -std=gnu99 -Wall -Wno-unused-variable -Wno-unused-parameter -Wno-unused-value -Wno-missing-braces
	CXX_FLAGS := -std=c++14 -Wall -Wfloat-conversion
	INCLUDE_DIRS := -I../../Platform/kApi -I../../Gocator/GoSdk
	DEFINES := -DK_DEBUG -DGO_EMIT -DEXPERIMENTAL_FEATURES_ENABLED
	LINKER_FLAGS := -shared -Wl,-no-undefined -Wl,-rpath,'$$ORIGIN'
	LIB_DIRS := -L../../lib/linux_x64d
	LIBS := -Wl,--start-group -lc -lpthread -lrt -lm -lkApi -Wl,--end-group
	LDFLAGS := $(LINKER_FLAGS) $(LIBS) $(LIB_DIRS)

	OBJECTS := ../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkLib.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkDef.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAcceleratorMgr.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelerator.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoLayout.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAdvanced.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoMaterial.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoMultiplexBank.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoPartDetection.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoPartMatching.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoPartModel.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileGeneration.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoRecordingFilter.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoReplay.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoReplayCondition.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSection.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSections.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSensor.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSensorInfo.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSetup.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceGeneration.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSystem.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoTransform.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoTracheid.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoGeoCal.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoUtils.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAlgorithm.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelSensorPortAlloc.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoControl.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscovery.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoReceiver.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSerializer.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDataSet.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDataTypes.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscoveryExtInfo.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoHealth.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoOutput.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAnalog.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDigital.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoEthernet.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSerial.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurement.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurements.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtMeasurement.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoFeature.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoFeatures.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoTool.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParam.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParams.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtTool.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtToolDataOutput.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoTools.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileTools.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileToolUtils.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoRangeTools.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceTools.o \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceToolUtils.o
	DEP_FILES = ../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkLib.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkDef.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAcceleratorMgr.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelerator.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoLayout.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAdvanced.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoMaterial.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoMultiplexBank.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoPartDetection.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoPartMatching.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoPartModel.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileGeneration.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoRecordingFilter.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoReplay.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoReplayCondition.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSection.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSections.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSensor.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSensorInfo.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSetup.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceGeneration.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSystem.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoTransform.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoTracheid.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoGeoCal.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoUtils.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAlgorithm.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelSensorPortAlloc.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoControl.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscovery.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoReceiver.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSerializer.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDataSet.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDataTypes.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscoveryExtInfo.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoHealth.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoOutput.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoAnalog.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoDigital.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoEthernet.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSerial.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurement.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurements.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtMeasurement.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoFeature.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoFeatures.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoTool.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParam.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParams.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtTool.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoExtToolDataOutput.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoTools.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileTools.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileToolUtils.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoRangeTools.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceTools.d \
	../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceToolUtils.d
	TARGET_DEPS = ../../Platform/kApi/../../lib/linux_x64d/libkApi.so

endif

ifeq ($(config),Release)
	TARGET := ../../lib/linux_x64/libGoSdk.so
	INTERMEDIATES := 
	OBJ_DIR := ../../build/GoSdk-gnumk_linux_x64-Release
	PREBUILD := 
	POSTBUILD := 
	COMPILER_FLAGS := -O2 -fpic
	C_FLAGS := -std=gnu99 -Wall -Wno-unused-variable -Wno-unused-parameter -Wno-unused-value -Wno-missing-braces
	CXX_FLAGS := -std=c++14 -Wall -Wfloat-conversion
	INCLUDE_DIRS := -I../../Platform/kApi -I../../Gocator/GoSdk
	DEFINES := -DGO_EMIT -DEXPERIMENTAL_FEATURES_ENABLED
	LINKER_FLAGS := -shared -Wl,-no-undefined -Wl,-rpath,'$$ORIGIN'
	LIB_DIRS := -L../../lib/linux_x64
	LIBS := -Wl,--start-group -lc -lpthread -lrt -lm -lkApi -Wl,--end-group
	LDFLAGS := $(LINKER_FLAGS) $(LIBS) $(LIB_DIRS)

	OBJECTS := ../../build/GoSdk-gnumk_linux_x64-Release/GoSdkLib.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSdkDef.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAcceleratorMgr.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAccelerator.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoLayout.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAdvanced.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoMaterial.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoMultiplexBank.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoPartDetection.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoPartMatching.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoPartModel.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoProfileGeneration.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoRecordingFilter.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoReplay.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoReplayCondition.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSection.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSections.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSensor.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSensorInfo.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSetup.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceGeneration.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSystem.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoTransform.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoTracheid.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoGeoCal.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoUtils.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAlgorithm.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAccelSensorPortAlloc.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoControl.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDiscovery.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoReceiver.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSerializer.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDataSet.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDataTypes.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDiscoveryExtInfo.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoHealth.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoOutput.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAnalog.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDigital.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoEthernet.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSerial.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurement.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurements.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtMeasurement.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoFeature.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoFeatures.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoTool.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtParam.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtParams.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtTool.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtToolDataOutput.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoTools.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoProfileTools.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoProfileToolUtils.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoRangeTools.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceTools.o \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceToolUtils.o
	DEP_FILES = ../../build/GoSdk-gnumk_linux_x64-Release/GoSdkLib.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSdkDef.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAcceleratorMgr.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAccelerator.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoLayout.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAdvanced.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoMaterial.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoMultiplexBank.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoPartDetection.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoPartMatching.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoPartModel.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoProfileGeneration.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoRecordingFilter.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoReplay.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoReplayCondition.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSection.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSections.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSensor.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSensorInfo.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSetup.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceGeneration.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSystem.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoTransform.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoTracheid.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoGeoCal.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoUtils.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAlgorithm.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAccelSensorPortAlloc.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoControl.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDiscovery.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoReceiver.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSerializer.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDataSet.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDataTypes.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDiscoveryExtInfo.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoHealth.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoOutput.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoAnalog.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoDigital.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoEthernet.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSerial.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurement.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurements.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtMeasurement.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoFeature.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoFeatures.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoTool.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtParam.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtParams.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtTool.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoExtToolDataOutput.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoTools.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoProfileTools.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoProfileToolUtils.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoRangeTools.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceTools.d \
	../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceToolUtils.d
	TARGET_DEPS = ../../Platform/kApi/../../lib/linux_x64/libkApi.so

endif

.PHONY: all all-obj all-dep clean

all: $(OBJ_DIR)
	$(PREBUILD)
	$(SILENT) $(MAKE) -f GoSdk-Linux_X64.mk all-dep
	$(SILENT) $(MAKE) -f GoSdk-Linux_X64.mk all-obj

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

../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkLib.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkLib.d: GoSdk/GoSdkLib.c
	$(SILENT) $(info GccX64 GoSdk/GoSdkLib.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkLib.o -c GoSdk/GoSdkLib.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkDef.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkDef.d: GoSdk/GoSdkDef.c
	$(SILENT) $(info GccX64 GoSdk/GoSdkDef.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkDef.o -c GoSdk/GoSdkDef.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoAcceleratorMgr.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAcceleratorMgr.d: GoSdk/GoAcceleratorMgr.c
	$(SILENT) $(info GccX64 GoSdk/GoAcceleratorMgr.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAcceleratorMgr.o -c GoSdk/GoAcceleratorMgr.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelerator.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelerator.d: GoSdk/GoAccelerator.c
	$(SILENT) $(info GccX64 GoSdk/GoAccelerator.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelerator.o -c GoSdk/GoAccelerator.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoLayout.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoLayout.d: GoSdk/GoLayout.c
	$(SILENT) $(info GccX64 GoSdk/GoLayout.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoLayout.o -c GoSdk/GoLayout.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoAdvanced.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAdvanced.d: GoSdk/GoAdvanced.c
	$(SILENT) $(info GccX64 GoSdk/GoAdvanced.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAdvanced.o -c GoSdk/GoAdvanced.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoMaterial.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoMaterial.d: GoSdk/GoMaterial.c
	$(SILENT) $(info GccX64 GoSdk/GoMaterial.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoMaterial.o -c GoSdk/GoMaterial.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoMultiplexBank.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoMultiplexBank.d: GoSdk/GoMultiplexBank.c
	$(SILENT) $(info GccX64 GoSdk/GoMultiplexBank.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoMultiplexBank.o -c GoSdk/GoMultiplexBank.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoPartDetection.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoPartDetection.d: GoSdk/GoPartDetection.c
	$(SILENT) $(info GccX64 GoSdk/GoPartDetection.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoPartDetection.o -c GoSdk/GoPartDetection.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoPartMatching.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoPartMatching.d: GoSdk/GoPartMatching.c
	$(SILENT) $(info GccX64 GoSdk/GoPartMatching.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoPartMatching.o -c GoSdk/GoPartMatching.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoPartModel.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoPartModel.d: GoSdk/GoPartModel.c
	$(SILENT) $(info GccX64 GoSdk/GoPartModel.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoPartModel.o -c GoSdk/GoPartModel.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileGeneration.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileGeneration.d: GoSdk/GoProfileGeneration.c
	$(SILENT) $(info GccX64 GoSdk/GoProfileGeneration.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileGeneration.o -c GoSdk/GoProfileGeneration.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoRecordingFilter.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoRecordingFilter.d: GoSdk/GoRecordingFilter.c
	$(SILENT) $(info GccX64 GoSdk/GoRecordingFilter.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoRecordingFilter.o -c GoSdk/GoRecordingFilter.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoReplay.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoReplay.d: GoSdk/GoReplay.c
	$(SILENT) $(info GccX64 GoSdk/GoReplay.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoReplay.o -c GoSdk/GoReplay.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoReplayCondition.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoReplayCondition.d: GoSdk/GoReplayCondition.c
	$(SILENT) $(info GccX64 GoSdk/GoReplayCondition.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoReplayCondition.o -c GoSdk/GoReplayCondition.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSection.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSection.d: GoSdk/GoSection.c
	$(SILENT) $(info GccX64 GoSdk/GoSection.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSection.o -c GoSdk/GoSection.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSections.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSections.d: GoSdk/GoSections.c
	$(SILENT) $(info GccX64 GoSdk/GoSections.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSections.o -c GoSdk/GoSections.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSensor.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSensor.d: GoSdk/GoSensor.c
	$(SILENT) $(info GccX64 GoSdk/GoSensor.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSensor.o -c GoSdk/GoSensor.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSensorInfo.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSensorInfo.d: GoSdk/GoSensorInfo.c
	$(SILENT) $(info GccX64 GoSdk/GoSensorInfo.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSensorInfo.o -c GoSdk/GoSensorInfo.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSetup.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSetup.d: GoSdk/GoSetup.c
	$(SILENT) $(info GccX64 GoSdk/GoSetup.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSetup.o -c GoSdk/GoSetup.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceGeneration.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceGeneration.d: GoSdk/GoSurfaceGeneration.c
	$(SILENT) $(info GccX64 GoSdk/GoSurfaceGeneration.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceGeneration.o -c GoSdk/GoSurfaceGeneration.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSystem.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSystem.d: GoSdk/GoSystem.c
	$(SILENT) $(info GccX64 GoSdk/GoSystem.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSystem.o -c GoSdk/GoSystem.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoTransform.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoTransform.d: GoSdk/GoTransform.c
	$(SILENT) $(info GccX64 GoSdk/GoTransform.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoTransform.o -c GoSdk/GoTransform.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoTracheid.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoTracheid.d: GoSdk/GoTracheid.c
	$(SILENT) $(info GccX64 GoSdk/GoTracheid.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoTracheid.o -c GoSdk/GoTracheid.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoGeoCal.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoGeoCal.d: GoSdk/GoGeoCal.c
	$(SILENT) $(info GccX64 GoSdk/GoGeoCal.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoGeoCal.o -c GoSdk/GoGeoCal.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoUtils.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoUtils.d: GoSdk/GoUtils.c
	$(SILENT) $(info GccX64 GoSdk/GoUtils.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoUtils.o -c GoSdk/GoUtils.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoAlgorithm.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAlgorithm.d: GoSdk/GoAlgorithm.c
	$(SILENT) $(info GccX64 GoSdk/GoAlgorithm.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAlgorithm.o -c GoSdk/GoAlgorithm.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelSensorPortAlloc.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelSensorPortAlloc.d: GoSdk/Internal/GoAccelSensorPortAlloc.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoAccelSensorPortAlloc.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelSensorPortAlloc.o -c GoSdk/Internal/GoAccelSensorPortAlloc.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoControl.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoControl.d: GoSdk/Internal/GoControl.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoControl.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoControl.o -c GoSdk/Internal/GoControl.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscovery.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscovery.d: GoSdk/Internal/GoDiscovery.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoDiscovery.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscovery.o -c GoSdk/Internal/GoDiscovery.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoReceiver.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoReceiver.d: GoSdk/Internal/GoReceiver.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoReceiver.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoReceiver.o -c GoSdk/Internal/GoReceiver.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSerializer.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSerializer.d: GoSdk/Internal/GoSerializer.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoSerializer.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSerializer.o -c GoSdk/Internal/GoSerializer.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoDataSet.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDataSet.d: GoSdk/Messages/GoDataSet.c
	$(SILENT) $(info GccX64 GoSdk/Messages/GoDataSet.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDataSet.o -c GoSdk/Messages/GoDataSet.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoDataTypes.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDataTypes.d: GoSdk/Messages/GoDataTypes.c
	$(SILENT) $(info GccX64 GoSdk/Messages/GoDataTypes.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDataTypes.o -c GoSdk/Messages/GoDataTypes.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscoveryExtInfo.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscoveryExtInfo.d: GoSdk/Messages/GoDiscoveryExtInfo.c
	$(SILENT) $(info GccX64 GoSdk/Messages/GoDiscoveryExtInfo.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscoveryExtInfo.o -c GoSdk/Messages/GoDiscoveryExtInfo.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoHealth.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoHealth.d: GoSdk/Messages/GoHealth.c
	$(SILENT) $(info GccX64 GoSdk/Messages/GoHealth.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoHealth.o -c GoSdk/Messages/GoHealth.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoOutput.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoOutput.d: GoSdk/Outputs/GoOutput.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoOutput.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoOutput.o -c GoSdk/Outputs/GoOutput.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoAnalog.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAnalog.d: GoSdk/Outputs/GoAnalog.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoAnalog.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoAnalog.o -c GoSdk/Outputs/GoAnalog.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoDigital.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDigital.d: GoSdk/Outputs/GoDigital.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoDigital.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoDigital.o -c GoSdk/Outputs/GoDigital.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoEthernet.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoEthernet.d: GoSdk/Outputs/GoEthernet.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoEthernet.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoEthernet.o -c GoSdk/Outputs/GoEthernet.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSerial.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSerial.d: GoSdk/Outputs/GoSerial.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoSerial.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSerial.o -c GoSdk/Outputs/GoSerial.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurement.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurement.d: GoSdk/Tools/GoMeasurement.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoMeasurement.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurement.o -c GoSdk/Tools/GoMeasurement.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurements.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurements.d: GoSdk/Tools/GoMeasurements.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoMeasurements.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurements.o -c GoSdk/Tools/GoMeasurements.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoExtMeasurement.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtMeasurement.d: GoSdk/Tools/GoExtMeasurement.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtMeasurement.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtMeasurement.o -c GoSdk/Tools/GoExtMeasurement.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoFeature.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoFeature.d: GoSdk/Tools/GoFeature.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoFeature.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoFeature.o -c GoSdk/Tools/GoFeature.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoFeatures.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoFeatures.d: GoSdk/Tools/GoFeatures.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoFeatures.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoFeatures.o -c GoSdk/Tools/GoFeatures.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoTool.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoTool.d: GoSdk/Tools/GoTool.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoTool.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoTool.o -c GoSdk/Tools/GoTool.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParam.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParam.d: GoSdk/Tools/GoExtParam.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtParam.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParam.o -c GoSdk/Tools/GoExtParam.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParams.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParams.d: GoSdk/Tools/GoExtParams.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtParams.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParams.o -c GoSdk/Tools/GoExtParams.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoExtTool.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtTool.d: GoSdk/Tools/GoExtTool.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtTool.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtTool.o -c GoSdk/Tools/GoExtTool.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoExtToolDataOutput.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtToolDataOutput.d: GoSdk/Tools/GoExtToolDataOutput.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtToolDataOutput.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtToolDataOutput.o -c GoSdk/Tools/GoExtToolDataOutput.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoTools.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoTools.d: GoSdk/Tools/GoTools.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoTools.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoTools.o -c GoSdk/Tools/GoTools.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileTools.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileTools.d: GoSdk/Tools/GoProfileTools.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoProfileTools.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileTools.o -c GoSdk/Tools/GoProfileTools.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileToolUtils.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileToolUtils.d: GoSdk/Tools/GoProfileToolUtils.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoProfileToolUtils.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileToolUtils.o -c GoSdk/Tools/GoProfileToolUtils.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoRangeTools.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoRangeTools.d: GoSdk/Tools/GoRangeTools.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoRangeTools.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoRangeTools.o -c GoSdk/Tools/GoRangeTools.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceTools.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceTools.d: GoSdk/Tools/GoSurfaceTools.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoSurfaceTools.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceTools.o -c GoSdk/Tools/GoSurfaceTools.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceToolUtils.o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceToolUtils.d: GoSdk/Tools/GoSurfaceToolUtils.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoSurfaceToolUtils.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceToolUtils.o -c GoSdk/Tools/GoSurfaceToolUtils.c -MMD -MP

endif

ifeq ($(config),Release)

../../build/GoSdk-gnumk_linux_x64-Release/GoSdkLib.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSdkLib.d: GoSdk/GoSdkLib.c
	$(SILENT) $(info GccX64 GoSdk/GoSdkLib.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSdkLib.o -c GoSdk/GoSdkLib.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSdkDef.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSdkDef.d: GoSdk/GoSdkDef.c
	$(SILENT) $(info GccX64 GoSdk/GoSdkDef.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSdkDef.o -c GoSdk/GoSdkDef.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoAcceleratorMgr.o ../../build/GoSdk-gnumk_linux_x64-Release/GoAcceleratorMgr.d: GoSdk/GoAcceleratorMgr.c
	$(SILENT) $(info GccX64 GoSdk/GoAcceleratorMgr.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoAcceleratorMgr.o -c GoSdk/GoAcceleratorMgr.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoAccelerator.o ../../build/GoSdk-gnumk_linux_x64-Release/GoAccelerator.d: GoSdk/GoAccelerator.c
	$(SILENT) $(info GccX64 GoSdk/GoAccelerator.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoAccelerator.o -c GoSdk/GoAccelerator.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoLayout.o ../../build/GoSdk-gnumk_linux_x64-Release/GoLayout.d: GoSdk/GoLayout.c
	$(SILENT) $(info GccX64 GoSdk/GoLayout.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoLayout.o -c GoSdk/GoLayout.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoAdvanced.o ../../build/GoSdk-gnumk_linux_x64-Release/GoAdvanced.d: GoSdk/GoAdvanced.c
	$(SILENT) $(info GccX64 GoSdk/GoAdvanced.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoAdvanced.o -c GoSdk/GoAdvanced.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoMaterial.o ../../build/GoSdk-gnumk_linux_x64-Release/GoMaterial.d: GoSdk/GoMaterial.c
	$(SILENT) $(info GccX64 GoSdk/GoMaterial.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoMaterial.o -c GoSdk/GoMaterial.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoMultiplexBank.o ../../build/GoSdk-gnumk_linux_x64-Release/GoMultiplexBank.d: GoSdk/GoMultiplexBank.c
	$(SILENT) $(info GccX64 GoSdk/GoMultiplexBank.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoMultiplexBank.o -c GoSdk/GoMultiplexBank.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoPartDetection.o ../../build/GoSdk-gnumk_linux_x64-Release/GoPartDetection.d: GoSdk/GoPartDetection.c
	$(SILENT) $(info GccX64 GoSdk/GoPartDetection.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoPartDetection.o -c GoSdk/GoPartDetection.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoPartMatching.o ../../build/GoSdk-gnumk_linux_x64-Release/GoPartMatching.d: GoSdk/GoPartMatching.c
	$(SILENT) $(info GccX64 GoSdk/GoPartMatching.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoPartMatching.o -c GoSdk/GoPartMatching.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoPartModel.o ../../build/GoSdk-gnumk_linux_x64-Release/GoPartModel.d: GoSdk/GoPartModel.c
	$(SILENT) $(info GccX64 GoSdk/GoPartModel.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoPartModel.o -c GoSdk/GoPartModel.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoProfileGeneration.o ../../build/GoSdk-gnumk_linux_x64-Release/GoProfileGeneration.d: GoSdk/GoProfileGeneration.c
	$(SILENT) $(info GccX64 GoSdk/GoProfileGeneration.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoProfileGeneration.o -c GoSdk/GoProfileGeneration.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoRecordingFilter.o ../../build/GoSdk-gnumk_linux_x64-Release/GoRecordingFilter.d: GoSdk/GoRecordingFilter.c
	$(SILENT) $(info GccX64 GoSdk/GoRecordingFilter.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoRecordingFilter.o -c GoSdk/GoRecordingFilter.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoReplay.o ../../build/GoSdk-gnumk_linux_x64-Release/GoReplay.d: GoSdk/GoReplay.c
	$(SILENT) $(info GccX64 GoSdk/GoReplay.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoReplay.o -c GoSdk/GoReplay.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoReplayCondition.o ../../build/GoSdk-gnumk_linux_x64-Release/GoReplayCondition.d: GoSdk/GoReplayCondition.c
	$(SILENT) $(info GccX64 GoSdk/GoReplayCondition.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoReplayCondition.o -c GoSdk/GoReplayCondition.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSection.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSection.d: GoSdk/GoSection.c
	$(SILENT) $(info GccX64 GoSdk/GoSection.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSection.o -c GoSdk/GoSection.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSections.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSections.d: GoSdk/GoSections.c
	$(SILENT) $(info GccX64 GoSdk/GoSections.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSections.o -c GoSdk/GoSections.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSensor.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSensor.d: GoSdk/GoSensor.c
	$(SILENT) $(info GccX64 GoSdk/GoSensor.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSensor.o -c GoSdk/GoSensor.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSensorInfo.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSensorInfo.d: GoSdk/GoSensorInfo.c
	$(SILENT) $(info GccX64 GoSdk/GoSensorInfo.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSensorInfo.o -c GoSdk/GoSensorInfo.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSetup.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSetup.d: GoSdk/GoSetup.c
	$(SILENT) $(info GccX64 GoSdk/GoSetup.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSetup.o -c GoSdk/GoSetup.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceGeneration.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceGeneration.d: GoSdk/GoSurfaceGeneration.c
	$(SILENT) $(info GccX64 GoSdk/GoSurfaceGeneration.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceGeneration.o -c GoSdk/GoSurfaceGeneration.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSystem.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSystem.d: GoSdk/GoSystem.c
	$(SILENT) $(info GccX64 GoSdk/GoSystem.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSystem.o -c GoSdk/GoSystem.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoTransform.o ../../build/GoSdk-gnumk_linux_x64-Release/GoTransform.d: GoSdk/GoTransform.c
	$(SILENT) $(info GccX64 GoSdk/GoTransform.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoTransform.o -c GoSdk/GoTransform.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoTracheid.o ../../build/GoSdk-gnumk_linux_x64-Release/GoTracheid.d: GoSdk/GoTracheid.c
	$(SILENT) $(info GccX64 GoSdk/GoTracheid.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoTracheid.o -c GoSdk/GoTracheid.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoGeoCal.o ../../build/GoSdk-gnumk_linux_x64-Release/GoGeoCal.d: GoSdk/GoGeoCal.c
	$(SILENT) $(info GccX64 GoSdk/GoGeoCal.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoGeoCal.o -c GoSdk/GoGeoCal.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoUtils.o ../../build/GoSdk-gnumk_linux_x64-Release/GoUtils.d: GoSdk/GoUtils.c
	$(SILENT) $(info GccX64 GoSdk/GoUtils.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoUtils.o -c GoSdk/GoUtils.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoAlgorithm.o ../../build/GoSdk-gnumk_linux_x64-Release/GoAlgorithm.d: GoSdk/GoAlgorithm.c
	$(SILENT) $(info GccX64 GoSdk/GoAlgorithm.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoAlgorithm.o -c GoSdk/GoAlgorithm.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoAccelSensorPortAlloc.o ../../build/GoSdk-gnumk_linux_x64-Release/GoAccelSensorPortAlloc.d: GoSdk/Internal/GoAccelSensorPortAlloc.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoAccelSensorPortAlloc.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoAccelSensorPortAlloc.o -c GoSdk/Internal/GoAccelSensorPortAlloc.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoControl.o ../../build/GoSdk-gnumk_linux_x64-Release/GoControl.d: GoSdk/Internal/GoControl.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoControl.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoControl.o -c GoSdk/Internal/GoControl.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoDiscovery.o ../../build/GoSdk-gnumk_linux_x64-Release/GoDiscovery.d: GoSdk/Internal/GoDiscovery.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoDiscovery.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoDiscovery.o -c GoSdk/Internal/GoDiscovery.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoReceiver.o ../../build/GoSdk-gnumk_linux_x64-Release/GoReceiver.d: GoSdk/Internal/GoReceiver.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoReceiver.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoReceiver.o -c GoSdk/Internal/GoReceiver.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSerializer.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSerializer.d: GoSdk/Internal/GoSerializer.c
	$(SILENT) $(info GccX64 GoSdk/Internal/GoSerializer.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSerializer.o -c GoSdk/Internal/GoSerializer.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoDataSet.o ../../build/GoSdk-gnumk_linux_x64-Release/GoDataSet.d: GoSdk/Messages/GoDataSet.c
	$(SILENT) $(info GccX64 GoSdk/Messages/GoDataSet.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoDataSet.o -c GoSdk/Messages/GoDataSet.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoDataTypes.o ../../build/GoSdk-gnumk_linux_x64-Release/GoDataTypes.d: GoSdk/Messages/GoDataTypes.c
	$(SILENT) $(info GccX64 GoSdk/Messages/GoDataTypes.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoDataTypes.o -c GoSdk/Messages/GoDataTypes.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoDiscoveryExtInfo.o ../../build/GoSdk-gnumk_linux_x64-Release/GoDiscoveryExtInfo.d: GoSdk/Messages/GoDiscoveryExtInfo.c
	$(SILENT) $(info GccX64 GoSdk/Messages/GoDiscoveryExtInfo.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoDiscoveryExtInfo.o -c GoSdk/Messages/GoDiscoveryExtInfo.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoHealth.o ../../build/GoSdk-gnumk_linux_x64-Release/GoHealth.d: GoSdk/Messages/GoHealth.c
	$(SILENT) $(info GccX64 GoSdk/Messages/GoHealth.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoHealth.o -c GoSdk/Messages/GoHealth.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoOutput.o ../../build/GoSdk-gnumk_linux_x64-Release/GoOutput.d: GoSdk/Outputs/GoOutput.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoOutput.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoOutput.o -c GoSdk/Outputs/GoOutput.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoAnalog.o ../../build/GoSdk-gnumk_linux_x64-Release/GoAnalog.d: GoSdk/Outputs/GoAnalog.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoAnalog.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoAnalog.o -c GoSdk/Outputs/GoAnalog.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoDigital.o ../../build/GoSdk-gnumk_linux_x64-Release/GoDigital.d: GoSdk/Outputs/GoDigital.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoDigital.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoDigital.o -c GoSdk/Outputs/GoDigital.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoEthernet.o ../../build/GoSdk-gnumk_linux_x64-Release/GoEthernet.d: GoSdk/Outputs/GoEthernet.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoEthernet.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoEthernet.o -c GoSdk/Outputs/GoEthernet.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSerial.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSerial.d: GoSdk/Outputs/GoSerial.c
	$(SILENT) $(info GccX64 GoSdk/Outputs/GoSerial.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSerial.o -c GoSdk/Outputs/GoSerial.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurement.o ../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurement.d: GoSdk/Tools/GoMeasurement.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoMeasurement.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurement.o -c GoSdk/Tools/GoMeasurement.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurements.o ../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurements.d: GoSdk/Tools/GoMeasurements.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoMeasurements.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurements.o -c GoSdk/Tools/GoMeasurements.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoExtMeasurement.o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtMeasurement.d: GoSdk/Tools/GoExtMeasurement.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtMeasurement.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtMeasurement.o -c GoSdk/Tools/GoExtMeasurement.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoFeature.o ../../build/GoSdk-gnumk_linux_x64-Release/GoFeature.d: GoSdk/Tools/GoFeature.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoFeature.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoFeature.o -c GoSdk/Tools/GoFeature.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoFeatures.o ../../build/GoSdk-gnumk_linux_x64-Release/GoFeatures.d: GoSdk/Tools/GoFeatures.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoFeatures.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoFeatures.o -c GoSdk/Tools/GoFeatures.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoTool.o ../../build/GoSdk-gnumk_linux_x64-Release/GoTool.d: GoSdk/Tools/GoTool.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoTool.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoTool.o -c GoSdk/Tools/GoTool.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoExtParam.o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtParam.d: GoSdk/Tools/GoExtParam.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtParam.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtParam.o -c GoSdk/Tools/GoExtParam.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoExtParams.o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtParams.d: GoSdk/Tools/GoExtParams.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtParams.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtParams.o -c GoSdk/Tools/GoExtParams.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoExtTool.o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtTool.d: GoSdk/Tools/GoExtTool.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtTool.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtTool.o -c GoSdk/Tools/GoExtTool.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoExtToolDataOutput.o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtToolDataOutput.d: GoSdk/Tools/GoExtToolDataOutput.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoExtToolDataOutput.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoExtToolDataOutput.o -c GoSdk/Tools/GoExtToolDataOutput.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoTools.o ../../build/GoSdk-gnumk_linux_x64-Release/GoTools.d: GoSdk/Tools/GoTools.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoTools.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoTools.o -c GoSdk/Tools/GoTools.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoProfileTools.o ../../build/GoSdk-gnumk_linux_x64-Release/GoProfileTools.d: GoSdk/Tools/GoProfileTools.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoProfileTools.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoProfileTools.o -c GoSdk/Tools/GoProfileTools.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoProfileToolUtils.o ../../build/GoSdk-gnumk_linux_x64-Release/GoProfileToolUtils.d: GoSdk/Tools/GoProfileToolUtils.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoProfileToolUtils.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoProfileToolUtils.o -c GoSdk/Tools/GoProfileToolUtils.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoRangeTools.o ../../build/GoSdk-gnumk_linux_x64-Release/GoRangeTools.d: GoSdk/Tools/GoRangeTools.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoRangeTools.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoRangeTools.o -c GoSdk/Tools/GoRangeTools.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceTools.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceTools.d: GoSdk/Tools/GoSurfaceTools.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoSurfaceTools.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceTools.o -c GoSdk/Tools/GoSurfaceTools.c -MMD -MP

../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceToolUtils.o ../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceToolUtils.d: GoSdk/Tools/GoSurfaceToolUtils.c
	$(SILENT) $(info GccX64 GoSdk/Tools/GoSurfaceToolUtils.c)
	$(SILENT) $(C_COMPILER) $(COMPILER_FLAGS) $(C_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceToolUtils.o -c GoSdk/Tools/GoSurfaceToolUtils.c -MMD -MP

endif

ifeq ($(MAKECMDGOALS),all-obj)

ifeq ($(config),Debug)

include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkLib.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSdkDef.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoAcceleratorMgr.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelerator.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoLayout.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoAdvanced.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoMaterial.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoMultiplexBank.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoPartDetection.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoPartMatching.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoPartModel.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileGeneration.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoRecordingFilter.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoReplay.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoReplayCondition.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSection.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSections.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSensor.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSensorInfo.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSetup.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceGeneration.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSystem.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoTransform.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoTracheid.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoGeoCal.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoUtils.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoAlgorithm.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoAccelSensorPortAlloc.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoControl.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscovery.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoReceiver.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSerializer.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoDataSet.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoDataTypes.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoDiscoveryExtInfo.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoHealth.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoOutput.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoAnalog.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoDigital.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoEthernet.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSerial.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurement.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoMeasurements.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtMeasurement.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoFeature.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoFeatures.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoTool.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParam.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtParams.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtTool.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoExtToolDataOutput.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoTools.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileTools.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoProfileToolUtils.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoRangeTools.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceTools.d
include ../../build/GoSdk-gnumk_linux_x64-Debug/GoSurfaceToolUtils.d

endif

ifeq ($(config),Release)

include ../../build/GoSdk-gnumk_linux_x64-Release/GoSdkLib.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSdkDef.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoAcceleratorMgr.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoAccelerator.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoLayout.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoAdvanced.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoMaterial.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoMultiplexBank.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoPartDetection.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoPartMatching.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoPartModel.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoProfileGeneration.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoRecordingFilter.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoReplay.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoReplayCondition.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSection.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSections.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSensor.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSensorInfo.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSetup.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceGeneration.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSystem.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoTransform.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoTracheid.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoGeoCal.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoUtils.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoAlgorithm.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoAccelSensorPortAlloc.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoControl.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoDiscovery.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoReceiver.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSerializer.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoDataSet.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoDataTypes.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoDiscoveryExtInfo.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoHealth.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoOutput.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoAnalog.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoDigital.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoEthernet.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSerial.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurement.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoMeasurements.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoExtMeasurement.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoFeature.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoFeatures.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoTool.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoExtParam.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoExtParams.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoExtTool.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoExtToolDataOutput.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoTools.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoProfileTools.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoProfileToolUtils.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoRangeTools.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceTools.d
include ../../build/GoSdk-gnumk_linux_x64-Release/GoSurfaceToolUtils.d

endif

endif

