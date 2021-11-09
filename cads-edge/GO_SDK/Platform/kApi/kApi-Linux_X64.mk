
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
	TARGET := ../../lib/linux_x64d/libkApi.so
	INTERMEDIATES := 
	OBJ_DIR := ../../build/kApi-gnumk_linux_x64-Debug
	PREBUILD := 
	POSTBUILD := 
	COMPILER_FLAGS := -g -fpic
	C_FLAGS := -std=gnu99 -Wall -Wno-unused-variable -Wno-unused-parameter -Wno-unused-value -Wno-missing-braces
	CXX_FLAGS := -std=c++11 -Wall
	INCLUDE_DIRS := -I../kApi
	DEFINES := -DK_DEBUG -DK_EMIT -DK_PLUGIN
	LINKER_FLAGS := -shared -Wl,-no-undefined -Wl,-rpath,'$$ORIGIN'
	LIB_DIRS :=
	LIBS := -Wl,--start-group -lc -lpthread -lrt -lm -ldl -Wl,--end-group
	LDFLAGS := $(LINKER_FLAGS) $(LIBS) $(LIB_DIRS)

	OBJECTS := ../../build/kApi-gnumk_linux_x64-Debug/kAlloc.o \
	../../build/kApi-gnumk_linux_x64-Debug/kApiDef.o \
	../../build/kApi-gnumk_linux_x64-Debug/kApiLib.o \
	../../build/kApi-gnumk_linux_x64-Debug/kAssembly.o \
	../../build/kApi-gnumk_linux_x64-Debug/kObject.o \
	../../build/kApi-gnumk_linux_x64-Debug/kType.o \
	../../build/kApi-gnumk_linux_x64-Debug/kValue.o \
	../../build/kApi-gnumk_linux_x64-Debug/kCipher.o \
	../../build/kApi-gnumk_linux_x64-Debug/kBlowfishCipher.o \
	../../build/kApi-gnumk_linux_x64-Debug/kCipherStream.o \
	../../build/kApi-gnumk_linux_x64-Debug/kHash.o \
	../../build/kApi-gnumk_linux_x64-Debug/kSha1Hash.o \
	../../build/kApi-gnumk_linux_x64-Debug/kArray1.o \
	../../build/kApi-gnumk_linux_x64-Debug/kArray2.o \
	../../build/kApi-gnumk_linux_x64-Debug/kArray3.o \
	../../build/kApi-gnumk_linux_x64-Debug/kArrayList.o \
	../../build/kApi-gnumk_linux_x64-Debug/kBox.o \
	../../build/kApi-gnumk_linux_x64-Debug/kBytes.o \
	../../build/kApi-gnumk_linux_x64-Debug/kCollection.o \
	../../build/kApi-gnumk_linux_x64-Debug/kImage.o \
	../../build/kApi-gnumk_linux_x64-Debug/kList.o \
	../../build/kApi-gnumk_linux_x64-Debug/kMath.o \
	../../build/kApi-gnumk_linux_x64-Debug/kMap.o \
	../../build/kApi-gnumk_linux_x64-Debug/kString.o \
	../../build/kApi-gnumk_linux_x64-Debug/kQueue.o \
	../../build/kApi-gnumk_linux_x64-Debug/kXml.o \
	../../build/kApi-gnumk_linux_x64-Debug/kDat5Serializer.o \
	../../build/kApi-gnumk_linux_x64-Debug/kDat6Serializer.o \
	../../build/kApi-gnumk_linux_x64-Debug/kDirectory.o \
	../../build/kApi-gnumk_linux_x64-Debug/kFile.o \
	../../build/kApi-gnumk_linux_x64-Debug/kHttpServer.o \
	../../build/kApi-gnumk_linux_x64-Debug/kHttpServerChannel.o \
	../../build/kApi-gnumk_linux_x64-Debug/kHttpServerRequest.o \
	../../build/kApi-gnumk_linux_x64-Debug/kHttpServerResponse.o \
	../../build/kApi-gnumk_linux_x64-Debug/kMemory.o \
	../../build/kApi-gnumk_linux_x64-Debug/kNetwork.o \
	../../build/kApi-gnumk_linux_x64-Debug/kPath.o \
	../../build/kApi-gnumk_linux_x64-Debug/kPipeStream.o \
	../../build/kApi-gnumk_linux_x64-Debug/kSerializer.o \
	../../build/kApi-gnumk_linux_x64-Debug/kStream.o \
	../../build/kApi-gnumk_linux_x64-Debug/kSocket.o \
	../../build/kApi-gnumk_linux_x64-Debug/kTcpClient.o \
	../../build/kApi-gnumk_linux_x64-Debug/kTcpServer.o \
	../../build/kApi-gnumk_linux_x64-Debug/kUdpClient.o \
	../../build/kApi-gnumk_linux_x64-Debug/kWebSocket.o \
	../../build/kApi-gnumk_linux_x64-Debug/kAtomic.o \
	../../build/kApi-gnumk_linux_x64-Debug/kLock.o \
	../../build/kApi-gnumk_linux_x64-Debug/kMsgQueue.o \
	../../build/kApi-gnumk_linux_x64-Debug/kParallel.o \
	../../build/kApi-gnumk_linux_x64-Debug/kPeriodic.o \
	../../build/kApi-gnumk_linux_x64-Debug/kThread.o \
	../../build/kApi-gnumk_linux_x64-Debug/kThreadPool.o \
	../../build/kApi-gnumk_linux_x64-Debug/kTimer.o \
	../../build/kApi-gnumk_linux_x64-Debug/kSemaphore.o \
	../../build/kApi-gnumk_linux_x64-Debug/kBackTrace.o \
	../../build/kApi-gnumk_linux_x64-Debug/kDateTime.o \
	../../build/kApi-gnumk_linux_x64-Debug/kDebugAlloc.o \
	../../build/kApi-gnumk_linux_x64-Debug/kDynamicLib.o \
	../../build/kApi-gnumk_linux_x64-Debug/kEvent.o \
	../../build/kApi-gnumk_linux_x64-Debug/kObjectPool.o \
	../../build/kApi-gnumk_linux_x64-Debug/kPlugin.o \
	../../build/kApi-gnumk_linux_x64-Debug/kPoolAlloc.o \
	../../build/kApi-gnumk_linux_x64-Debug/kProcess.o \
	../../build/kApi-gnumk_linux_x64-Debug/kSymbolInfo.o \
	../../build/kApi-gnumk_linux_x64-Debug/kTimeSpan.o \
	../../build/kApi-gnumk_linux_x64-Debug/kUserAlloc.o \
	../../build/kApi-gnumk_linux_x64-Debug/kUtils.o
	DEP_FILES = ../../build/kApi-gnumk_linux_x64-Debug/kAlloc.d \
	../../build/kApi-gnumk_linux_x64-Debug/kApiDef.d \
	../../build/kApi-gnumk_linux_x64-Debug/kApiLib.d \
	../../build/kApi-gnumk_linux_x64-Debug/kAssembly.d \
	../../build/kApi-gnumk_linux_x64-Debug/kObject.d \
	../../build/kApi-gnumk_linux_x64-Debug/kType.d \
	../../build/kApi-gnumk_linux_x64-Debug/kValue.d \
	../../build/kApi-gnumk_linux_x64-Debug/kCipher.d \
	../../build/kApi-gnumk_linux_x64-Debug/kBlowfishCipher.d \
	../../build/kApi-gnumk_linux_x64-Debug/kCipherStream.d \
	../../build/kApi-gnumk_linux_x64-Debug/kHash.d \
	../../build/kApi-gnumk_linux_x64-Debug/kSha1Hash.d \
	../../build/kApi-gnumk_linux_x64-Debug/kArray1.d \
	../../build/kApi-gnumk_linux_x64-Debug/kArray2.d \
	../../build/kApi-gnumk_linux_x64-Debug/kArray3.d \
	../../build/kApi-gnumk_linux_x64-Debug/kArrayList.d \
	../../build/kApi-gnumk_linux_x64-Debug/kBox.d \
	../../build/kApi-gnumk_linux_x64-Debug/kBytes.d \
	../../build/kApi-gnumk_linux_x64-Debug/kCollection.d \
	../../build/kApi-gnumk_linux_x64-Debug/kImage.d \
	../../build/kApi-gnumk_linux_x64-Debug/kList.d \
	../../build/kApi-gnumk_linux_x64-Debug/kMath.d \
	../../build/kApi-gnumk_linux_x64-Debug/kMap.d \
	../../build/kApi-gnumk_linux_x64-Debug/kString.d \
	../../build/kApi-gnumk_linux_x64-Debug/kQueue.d \
	../../build/kApi-gnumk_linux_x64-Debug/kXml.d \
	../../build/kApi-gnumk_linux_x64-Debug/kDat5Serializer.d \
	../../build/kApi-gnumk_linux_x64-Debug/kDat6Serializer.d \
	../../build/kApi-gnumk_linux_x64-Debug/kDirectory.d \
	../../build/kApi-gnumk_linux_x64-Debug/kFile.d \
	../../build/kApi-gnumk_linux_x64-Debug/kHttpServer.d \
	../../build/kApi-gnumk_linux_x64-Debug/kHttpServerChannel.d \
	../../build/kApi-gnumk_linux_x64-Debug/kHttpServerRequest.d \
	../../build/kApi-gnumk_linux_x64-Debug/kHttpServerResponse.d \
	../../build/kApi-gnumk_linux_x64-Debug/kMemory.d \
	../../build/kApi-gnumk_linux_x64-Debug/kNetwork.d \
	../../build/kApi-gnumk_linux_x64-Debug/kPath.d \
	../../build/kApi-gnumk_linux_x64-Debug/kPipeStream.d \
	../../build/kApi-gnumk_linux_x64-Debug/kSerializer.d \
	../../build/kApi-gnumk_linux_x64-Debug/kStream.d \
	../../build/kApi-gnumk_linux_x64-Debug/kSocket.d \
	../../build/kApi-gnumk_linux_x64-Debug/kTcpClient.d \
	../../build/kApi-gnumk_linux_x64-Debug/kTcpServer.d \
	../../build/kApi-gnumk_linux_x64-Debug/kUdpClient.d \
	../../build/kApi-gnumk_linux_x64-Debug/kWebSocket.d \
	../../build/kApi-gnumk_linux_x64-Debug/kAtomic.d \
	../../build/kApi-gnumk_linux_x64-Debug/kLock.d \
	../../build/kApi-gnumk_linux_x64-Debug/kMsgQueue.d \
	../../build/kApi-gnumk_linux_x64-Debug/kParallel.d \
	../../build/kApi-gnumk_linux_x64-Debug/kPeriodic.d \
	../../build/kApi-gnumk_linux_x64-Debug/kThread.d \
	../../build/kApi-gnumk_linux_x64-Debug/kThreadPool.d \
	../../build/kApi-gnumk_linux_x64-Debug/kTimer.d \
	../../build/kApi-gnumk_linux_x64-Debug/kSemaphore.d \
	../../build/kApi-gnumk_linux_x64-Debug/kBackTrace.d \
	../../build/kApi-gnumk_linux_x64-Debug/kDateTime.d \
	../../build/kApi-gnumk_linux_x64-Debug/kDebugAlloc.d \
	../../build/kApi-gnumk_linux_x64-Debug/kDynamicLib.d \
	../../build/kApi-gnumk_linux_x64-Debug/kEvent.d \
	../../build/kApi-gnumk_linux_x64-Debug/kObjectPool.d \
	../../build/kApi-gnumk_linux_x64-Debug/kPlugin.d \
	../../build/kApi-gnumk_linux_x64-Debug/kPoolAlloc.d \
	../../build/kApi-gnumk_linux_x64-Debug/kProcess.d \
	../../build/kApi-gnumk_linux_x64-Debug/kSymbolInfo.d \
	../../build/kApi-gnumk_linux_x64-Debug/kTimeSpan.d \
	../../build/kApi-gnumk_linux_x64-Debug/kUserAlloc.d \
	../../build/kApi-gnumk_linux_x64-Debug/kUtils.d
	TARGET_DEPS = 

endif

ifeq ($(config),Release)
	TARGET := ../../lib/linux_x64/libkApi.so
	INTERMEDIATES := 
	OBJ_DIR := ../../build/kApi-gnumk_linux_x64-Release
	PREBUILD := 
	POSTBUILD := 
	COMPILER_FLAGS := -O2 -fpic
	C_FLAGS := -std=gnu99 -Wall -Wno-unused-variable -Wno-unused-parameter -Wno-unused-value -Wno-missing-braces
	CXX_FLAGS := -std=c++11 -Wall
	INCLUDE_DIRS := -I../kApi
	DEFINES := -DK_EMIT -DK_PLUGIN
	LINKER_FLAGS := -shared -Wl,-no-undefined -Wl,-rpath,'$$ORIGIN'
	LIB_DIRS :=
	LIBS := -Wl,--start-group -lc -lpthread -lrt -lm -ldl -Wl,--end-group
	LDFLAGS := $(LINKER_FLAGS) $(LIBS) $(LIB_DIRS)

	OBJECTS := ../../build/kApi-gnumk_linux_x64-Release/kAlloc.o \
	../../build/kApi-gnumk_linux_x64-Release/kApiDef.o \
	../../build/kApi-gnumk_linux_x64-Release/kApiLib.o \
	../../build/kApi-gnumk_linux_x64-Release/kAssembly.o \
	../../build/kApi-gnumk_linux_x64-Release/kObject.o \
	../../build/kApi-gnumk_linux_x64-Release/kType.o \
	../../build/kApi-gnumk_linux_x64-Release/kValue.o \
	../../build/kApi-gnumk_linux_x64-Release/kCipher.o \
	../../build/kApi-gnumk_linux_x64-Release/kBlowfishCipher.o \
	../../build/kApi-gnumk_linux_x64-Release/kCipherStream.o \
	../../build/kApi-gnumk_linux_x64-Release/kHash.o \
	../../build/kApi-gnumk_linux_x64-Release/kSha1Hash.o \
	../../build/kApi-gnumk_linux_x64-Release/kArray1.o \
	../../build/kApi-gnumk_linux_x64-Release/kArray2.o \
	../../build/kApi-gnumk_linux_x64-Release/kArray3.o \
	../../build/kApi-gnumk_linux_x64-Release/kArrayList.o \
	../../build/kApi-gnumk_linux_x64-Release/kBox.o \
	../../build/kApi-gnumk_linux_x64-Release/kBytes.o \
	../../build/kApi-gnumk_linux_x64-Release/kCollection.o \
	../../build/kApi-gnumk_linux_x64-Release/kImage.o \
	../../build/kApi-gnumk_linux_x64-Release/kList.o \
	../../build/kApi-gnumk_linux_x64-Release/kMath.o \
	../../build/kApi-gnumk_linux_x64-Release/kMap.o \
	../../build/kApi-gnumk_linux_x64-Release/kString.o \
	../../build/kApi-gnumk_linux_x64-Release/kQueue.o \
	../../build/kApi-gnumk_linux_x64-Release/kXml.o \
	../../build/kApi-gnumk_linux_x64-Release/kDat5Serializer.o \
	../../build/kApi-gnumk_linux_x64-Release/kDat6Serializer.o \
	../../build/kApi-gnumk_linux_x64-Release/kDirectory.o \
	../../build/kApi-gnumk_linux_x64-Release/kFile.o \
	../../build/kApi-gnumk_linux_x64-Release/kHttpServer.o \
	../../build/kApi-gnumk_linux_x64-Release/kHttpServerChannel.o \
	../../build/kApi-gnumk_linux_x64-Release/kHttpServerRequest.o \
	../../build/kApi-gnumk_linux_x64-Release/kHttpServerResponse.o \
	../../build/kApi-gnumk_linux_x64-Release/kMemory.o \
	../../build/kApi-gnumk_linux_x64-Release/kNetwork.o \
	../../build/kApi-gnumk_linux_x64-Release/kPath.o \
	../../build/kApi-gnumk_linux_x64-Release/kPipeStream.o \
	../../build/kApi-gnumk_linux_x64-Release/kSerializer.o \
	../../build/kApi-gnumk_linux_x64-Release/kStream.o \
	../../build/kApi-gnumk_linux_x64-Release/kSocket.o \
	../../build/kApi-gnumk_linux_x64-Release/kTcpClient.o \
	../../build/kApi-gnumk_linux_x64-Release/kTcpServer.o \
	../../build/kApi-gnumk_linux_x64-Release/kUdpClient.o \
	../../build/kApi-gnumk_linux_x64-Release/kWebSocket.o \
	../../build/kApi-gnumk_linux_x64-Release/kAtomic.o \
	../../build/kApi-gnumk_linux_x64-Release/kLock.o \
	../../build/kApi-gnumk_linux_x64-Release/kMsgQueue.o \
	../../build/kApi-gnumk_linux_x64-Release/kParallel.o \
	../../build/kApi-gnumk_linux_x64-Release/kPeriodic.o \
	../../build/kApi-gnumk_linux_x64-Release/kThread.o \
	../../build/kApi-gnumk_linux_x64-Release/kThreadPool.o \
	../../build/kApi-gnumk_linux_x64-Release/kTimer.o \
	../../build/kApi-gnumk_linux_x64-Release/kSemaphore.o \
	../../build/kApi-gnumk_linux_x64-Release/kBackTrace.o \
	../../build/kApi-gnumk_linux_x64-Release/kDateTime.o \
	../../build/kApi-gnumk_linux_x64-Release/kDebugAlloc.o \
	../../build/kApi-gnumk_linux_x64-Release/kDynamicLib.o \
	../../build/kApi-gnumk_linux_x64-Release/kEvent.o \
	../../build/kApi-gnumk_linux_x64-Release/kObjectPool.o \
	../../build/kApi-gnumk_linux_x64-Release/kPlugin.o \
	../../build/kApi-gnumk_linux_x64-Release/kPoolAlloc.o \
	../../build/kApi-gnumk_linux_x64-Release/kProcess.o \
	../../build/kApi-gnumk_linux_x64-Release/kSymbolInfo.o \
	../../build/kApi-gnumk_linux_x64-Release/kTimeSpan.o \
	../../build/kApi-gnumk_linux_x64-Release/kUserAlloc.o \
	../../build/kApi-gnumk_linux_x64-Release/kUtils.o
	DEP_FILES = ../../build/kApi-gnumk_linux_x64-Release/kAlloc.d \
	../../build/kApi-gnumk_linux_x64-Release/kApiDef.d \
	../../build/kApi-gnumk_linux_x64-Release/kApiLib.d \
	../../build/kApi-gnumk_linux_x64-Release/kAssembly.d \
	../../build/kApi-gnumk_linux_x64-Release/kObject.d \
	../../build/kApi-gnumk_linux_x64-Release/kType.d \
	../../build/kApi-gnumk_linux_x64-Release/kValue.d \
	../../build/kApi-gnumk_linux_x64-Release/kCipher.d \
	../../build/kApi-gnumk_linux_x64-Release/kBlowfishCipher.d \
	../../build/kApi-gnumk_linux_x64-Release/kCipherStream.d \
	../../build/kApi-gnumk_linux_x64-Release/kHash.d \
	../../build/kApi-gnumk_linux_x64-Release/kSha1Hash.d \
	../../build/kApi-gnumk_linux_x64-Release/kArray1.d \
	../../build/kApi-gnumk_linux_x64-Release/kArray2.d \
	../../build/kApi-gnumk_linux_x64-Release/kArray3.d \
	../../build/kApi-gnumk_linux_x64-Release/kArrayList.d \
	../../build/kApi-gnumk_linux_x64-Release/kBox.d \
	../../build/kApi-gnumk_linux_x64-Release/kBytes.d \
	../../build/kApi-gnumk_linux_x64-Release/kCollection.d \
	../../build/kApi-gnumk_linux_x64-Release/kImage.d \
	../../build/kApi-gnumk_linux_x64-Release/kList.d \
	../../build/kApi-gnumk_linux_x64-Release/kMath.d \
	../../build/kApi-gnumk_linux_x64-Release/kMap.d \
	../../build/kApi-gnumk_linux_x64-Release/kString.d \
	../../build/kApi-gnumk_linux_x64-Release/kQueue.d \
	../../build/kApi-gnumk_linux_x64-Release/kXml.d \
	../../build/kApi-gnumk_linux_x64-Release/kDat5Serializer.d \
	../../build/kApi-gnumk_linux_x64-Release/kDat6Serializer.d \
	../../build/kApi-gnumk_linux_x64-Release/kDirectory.d \
	../../build/kApi-gnumk_linux_x64-Release/kFile.d \
	../../build/kApi-gnumk_linux_x64-Release/kHttpServer.d \
	../../build/kApi-gnumk_linux_x64-Release/kHttpServerChannel.d \
	../../build/kApi-gnumk_linux_x64-Release/kHttpServerRequest.d \
	../../build/kApi-gnumk_linux_x64-Release/kHttpServerResponse.d \
	../../build/kApi-gnumk_linux_x64-Release/kMemory.d \
	../../build/kApi-gnumk_linux_x64-Release/kNetwork.d \
	../../build/kApi-gnumk_linux_x64-Release/kPath.d \
	../../build/kApi-gnumk_linux_x64-Release/kPipeStream.d \
	../../build/kApi-gnumk_linux_x64-Release/kSerializer.d \
	../../build/kApi-gnumk_linux_x64-Release/kStream.d \
	../../build/kApi-gnumk_linux_x64-Release/kSocket.d \
	../../build/kApi-gnumk_linux_x64-Release/kTcpClient.d \
	../../build/kApi-gnumk_linux_x64-Release/kTcpServer.d \
	../../build/kApi-gnumk_linux_x64-Release/kUdpClient.d \
	../../build/kApi-gnumk_linux_x64-Release/kWebSocket.d \
	../../build/kApi-gnumk_linux_x64-Release/kAtomic.d \
	../../build/kApi-gnumk_linux_x64-Release/kLock.d \
	../../build/kApi-gnumk_linux_x64-Release/kMsgQueue.d \
	../../build/kApi-gnumk_linux_x64-Release/kParallel.d \
	../../build/kApi-gnumk_linux_x64-Release/kPeriodic.d \
	../../build/kApi-gnumk_linux_x64-Release/kThread.d \
	../../build/kApi-gnumk_linux_x64-Release/kThreadPool.d \
	../../build/kApi-gnumk_linux_x64-Release/kTimer.d \
	../../build/kApi-gnumk_linux_x64-Release/kSemaphore.d \
	../../build/kApi-gnumk_linux_x64-Release/kBackTrace.d \
	../../build/kApi-gnumk_linux_x64-Release/kDateTime.d \
	../../build/kApi-gnumk_linux_x64-Release/kDebugAlloc.d \
	../../build/kApi-gnumk_linux_x64-Release/kDynamicLib.d \
	../../build/kApi-gnumk_linux_x64-Release/kEvent.d \
	../../build/kApi-gnumk_linux_x64-Release/kObjectPool.d \
	../../build/kApi-gnumk_linux_x64-Release/kPlugin.d \
	../../build/kApi-gnumk_linux_x64-Release/kPoolAlloc.d \
	../../build/kApi-gnumk_linux_x64-Release/kProcess.d \
	../../build/kApi-gnumk_linux_x64-Release/kSymbolInfo.d \
	../../build/kApi-gnumk_linux_x64-Release/kTimeSpan.d \
	../../build/kApi-gnumk_linux_x64-Release/kUserAlloc.d \
	../../build/kApi-gnumk_linux_x64-Release/kUtils.d
	TARGET_DEPS = 

endif

.PHONY: all all-obj all-dep clean

all: $(OBJ_DIR)
	$(PREBUILD)
	$(SILENT) $(MAKE) -f kApi-Linux_X64.mk all-dep
	$(SILENT) $(MAKE) -f kApi-Linux_X64.mk all-obj

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

../../build/kApi-gnumk_linux_x64-Debug/kAlloc.o ../../build/kApi-gnumk_linux_x64-Debug/kAlloc.d: kApi/kAlloc.cpp
	$(SILENT) $(info GccX64 kApi/kAlloc.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kAlloc.o -c kApi/kAlloc.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kApiDef.o ../../build/kApi-gnumk_linux_x64-Debug/kApiDef.d: kApi/kApiDef.cpp
	$(SILENT) $(info GccX64 kApi/kApiDef.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kApiDef.o -c kApi/kApiDef.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kApiLib.o ../../build/kApi-gnumk_linux_x64-Debug/kApiLib.d: kApi/kApiLib.cpp
	$(SILENT) $(info GccX64 kApi/kApiLib.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kApiLib.o -c kApi/kApiLib.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kAssembly.o ../../build/kApi-gnumk_linux_x64-Debug/kAssembly.d: kApi/kAssembly.cpp
	$(SILENT) $(info GccX64 kApi/kAssembly.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kAssembly.o -c kApi/kAssembly.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kObject.o ../../build/kApi-gnumk_linux_x64-Debug/kObject.d: kApi/kObject.cpp
	$(SILENT) $(info GccX64 kApi/kObject.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kObject.o -c kApi/kObject.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kType.o ../../build/kApi-gnumk_linux_x64-Debug/kType.d: kApi/kType.cpp
	$(SILENT) $(info GccX64 kApi/kType.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kType.o -c kApi/kType.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kValue.o ../../build/kApi-gnumk_linux_x64-Debug/kValue.d: kApi/kValue.cpp
	$(SILENT) $(info GccX64 kApi/kValue.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kValue.o -c kApi/kValue.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kCipher.o ../../build/kApi-gnumk_linux_x64-Debug/kCipher.d: kApi/Crypto/kCipher.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kCipher.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kCipher.o -c kApi/Crypto/kCipher.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kBlowfishCipher.o ../../build/kApi-gnumk_linux_x64-Debug/kBlowfishCipher.d: kApi/Crypto/kBlowfishCipher.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kBlowfishCipher.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kBlowfishCipher.o -c kApi/Crypto/kBlowfishCipher.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kCipherStream.o ../../build/kApi-gnumk_linux_x64-Debug/kCipherStream.d: kApi/Crypto/kCipherStream.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kCipherStream.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kCipherStream.o -c kApi/Crypto/kCipherStream.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kHash.o ../../build/kApi-gnumk_linux_x64-Debug/kHash.d: kApi/Crypto/kHash.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kHash.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kHash.o -c kApi/Crypto/kHash.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kSha1Hash.o ../../build/kApi-gnumk_linux_x64-Debug/kSha1Hash.d: kApi/Crypto/kSha1Hash.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kSha1Hash.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kSha1Hash.o -c kApi/Crypto/kSha1Hash.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kArray1.o ../../build/kApi-gnumk_linux_x64-Debug/kArray1.d: kApi/Data/kArray1.cpp
	$(SILENT) $(info GccX64 kApi/Data/kArray1.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kArray1.o -c kApi/Data/kArray1.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kArray2.o ../../build/kApi-gnumk_linux_x64-Debug/kArray2.d: kApi/Data/kArray2.cpp
	$(SILENT) $(info GccX64 kApi/Data/kArray2.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kArray2.o -c kApi/Data/kArray2.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kArray3.o ../../build/kApi-gnumk_linux_x64-Debug/kArray3.d: kApi/Data/kArray3.cpp
	$(SILENT) $(info GccX64 kApi/Data/kArray3.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kArray3.o -c kApi/Data/kArray3.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kArrayList.o ../../build/kApi-gnumk_linux_x64-Debug/kArrayList.d: kApi/Data/kArrayList.cpp
	$(SILENT) $(info GccX64 kApi/Data/kArrayList.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kArrayList.o -c kApi/Data/kArrayList.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kBox.o ../../build/kApi-gnumk_linux_x64-Debug/kBox.d: kApi/Data/kBox.cpp
	$(SILENT) $(info GccX64 kApi/Data/kBox.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kBox.o -c kApi/Data/kBox.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kBytes.o ../../build/kApi-gnumk_linux_x64-Debug/kBytes.d: kApi/Data/kBytes.cpp
	$(SILENT) $(info GccX64 kApi/Data/kBytes.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kBytes.o -c kApi/Data/kBytes.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kCollection.o ../../build/kApi-gnumk_linux_x64-Debug/kCollection.d: kApi/Data/kCollection.cpp
	$(SILENT) $(info GccX64 kApi/Data/kCollection.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kCollection.o -c kApi/Data/kCollection.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kImage.o ../../build/kApi-gnumk_linux_x64-Debug/kImage.d: kApi/Data/kImage.cpp
	$(SILENT) $(info GccX64 kApi/Data/kImage.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kImage.o -c kApi/Data/kImage.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kList.o ../../build/kApi-gnumk_linux_x64-Debug/kList.d: kApi/Data/kList.cpp
	$(SILENT) $(info GccX64 kApi/Data/kList.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kList.o -c kApi/Data/kList.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kMath.o ../../build/kApi-gnumk_linux_x64-Debug/kMath.d: kApi/Data/kMath.cpp
	$(SILENT) $(info GccX64 kApi/Data/kMath.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kMath.o -c kApi/Data/kMath.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kMap.o ../../build/kApi-gnumk_linux_x64-Debug/kMap.d: kApi/Data/kMap.cpp
	$(SILENT) $(info GccX64 kApi/Data/kMap.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kMap.o -c kApi/Data/kMap.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kString.o ../../build/kApi-gnumk_linux_x64-Debug/kString.d: kApi/Data/kString.cpp
	$(SILENT) $(info GccX64 kApi/Data/kString.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kString.o -c kApi/Data/kString.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kQueue.o ../../build/kApi-gnumk_linux_x64-Debug/kQueue.d: kApi/Data/kQueue.cpp
	$(SILENT) $(info GccX64 kApi/Data/kQueue.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kQueue.o -c kApi/Data/kQueue.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kXml.o ../../build/kApi-gnumk_linux_x64-Debug/kXml.d: kApi/Data/kXml.cpp
	$(SILENT) $(info GccX64 kApi/Data/kXml.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kXml.o -c kApi/Data/kXml.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kDat5Serializer.o ../../build/kApi-gnumk_linux_x64-Debug/kDat5Serializer.d: kApi/Io/kDat5Serializer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kDat5Serializer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kDat5Serializer.o -c kApi/Io/kDat5Serializer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kDat6Serializer.o ../../build/kApi-gnumk_linux_x64-Debug/kDat6Serializer.d: kApi/Io/kDat6Serializer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kDat6Serializer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kDat6Serializer.o -c kApi/Io/kDat6Serializer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kDirectory.o ../../build/kApi-gnumk_linux_x64-Debug/kDirectory.d: kApi/Io/kDirectory.cpp
	$(SILENT) $(info GccX64 kApi/Io/kDirectory.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kDirectory.o -c kApi/Io/kDirectory.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kFile.o ../../build/kApi-gnumk_linux_x64-Debug/kFile.d: kApi/Io/kFile.cpp
	$(SILENT) $(info GccX64 kApi/Io/kFile.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kFile.o -c kApi/Io/kFile.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kHttpServer.o ../../build/kApi-gnumk_linux_x64-Debug/kHttpServer.d: kApi/Io/kHttpServer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kHttpServer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kHttpServer.o -c kApi/Io/kHttpServer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kHttpServerChannel.o ../../build/kApi-gnumk_linux_x64-Debug/kHttpServerChannel.d: kApi/Io/kHttpServerChannel.cpp
	$(SILENT) $(info GccX64 kApi/Io/kHttpServerChannel.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kHttpServerChannel.o -c kApi/Io/kHttpServerChannel.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kHttpServerRequest.o ../../build/kApi-gnumk_linux_x64-Debug/kHttpServerRequest.d: kApi/Io/kHttpServerRequest.cpp
	$(SILENT) $(info GccX64 kApi/Io/kHttpServerRequest.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kHttpServerRequest.o -c kApi/Io/kHttpServerRequest.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kHttpServerResponse.o ../../build/kApi-gnumk_linux_x64-Debug/kHttpServerResponse.d: kApi/Io/kHttpServerResponse.cpp
	$(SILENT) $(info GccX64 kApi/Io/kHttpServerResponse.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kHttpServerResponse.o -c kApi/Io/kHttpServerResponse.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kMemory.o ../../build/kApi-gnumk_linux_x64-Debug/kMemory.d: kApi/Io/kMemory.cpp
	$(SILENT) $(info GccX64 kApi/Io/kMemory.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kMemory.o -c kApi/Io/kMemory.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kNetwork.o ../../build/kApi-gnumk_linux_x64-Debug/kNetwork.d: kApi/Io/kNetwork.cpp
	$(SILENT) $(info GccX64 kApi/Io/kNetwork.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kNetwork.o -c kApi/Io/kNetwork.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kPath.o ../../build/kApi-gnumk_linux_x64-Debug/kPath.d: kApi/Io/kPath.cpp
	$(SILENT) $(info GccX64 kApi/Io/kPath.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kPath.o -c kApi/Io/kPath.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kPipeStream.o ../../build/kApi-gnumk_linux_x64-Debug/kPipeStream.d: kApi/Io/kPipeStream.cpp
	$(SILENT) $(info GccX64 kApi/Io/kPipeStream.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kPipeStream.o -c kApi/Io/kPipeStream.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kSerializer.o ../../build/kApi-gnumk_linux_x64-Debug/kSerializer.d: kApi/Io/kSerializer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kSerializer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kSerializer.o -c kApi/Io/kSerializer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kStream.o ../../build/kApi-gnumk_linux_x64-Debug/kStream.d: kApi/Io/kStream.cpp
	$(SILENT) $(info GccX64 kApi/Io/kStream.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kStream.o -c kApi/Io/kStream.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kSocket.o ../../build/kApi-gnumk_linux_x64-Debug/kSocket.d: kApi/Io/kSocket.cpp
	$(SILENT) $(info GccX64 kApi/Io/kSocket.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kSocket.o -c kApi/Io/kSocket.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kTcpClient.o ../../build/kApi-gnumk_linux_x64-Debug/kTcpClient.d: kApi/Io/kTcpClient.cpp
	$(SILENT) $(info GccX64 kApi/Io/kTcpClient.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kTcpClient.o -c kApi/Io/kTcpClient.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kTcpServer.o ../../build/kApi-gnumk_linux_x64-Debug/kTcpServer.d: kApi/Io/kTcpServer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kTcpServer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kTcpServer.o -c kApi/Io/kTcpServer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kUdpClient.o ../../build/kApi-gnumk_linux_x64-Debug/kUdpClient.d: kApi/Io/kUdpClient.cpp
	$(SILENT) $(info GccX64 kApi/Io/kUdpClient.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kUdpClient.o -c kApi/Io/kUdpClient.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kWebSocket.o ../../build/kApi-gnumk_linux_x64-Debug/kWebSocket.d: kApi/Io/kWebSocket.cpp
	$(SILENT) $(info GccX64 kApi/Io/kWebSocket.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kWebSocket.o -c kApi/Io/kWebSocket.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kAtomic.o ../../build/kApi-gnumk_linux_x64-Debug/kAtomic.d: kApi/Threads/kAtomic.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kAtomic.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kAtomic.o -c kApi/Threads/kAtomic.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kLock.o ../../build/kApi-gnumk_linux_x64-Debug/kLock.d: kApi/Threads/kLock.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kLock.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kLock.o -c kApi/Threads/kLock.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kMsgQueue.o ../../build/kApi-gnumk_linux_x64-Debug/kMsgQueue.d: kApi/Threads/kMsgQueue.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kMsgQueue.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kMsgQueue.o -c kApi/Threads/kMsgQueue.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kParallel.o ../../build/kApi-gnumk_linux_x64-Debug/kParallel.d: kApi/Threads/kParallel.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kParallel.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kParallel.o -c kApi/Threads/kParallel.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kPeriodic.o ../../build/kApi-gnumk_linux_x64-Debug/kPeriodic.d: kApi/Threads/kPeriodic.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kPeriodic.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kPeriodic.o -c kApi/Threads/kPeriodic.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kThread.o ../../build/kApi-gnumk_linux_x64-Debug/kThread.d: kApi/Threads/kThread.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kThread.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kThread.o -c kApi/Threads/kThread.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kThreadPool.o ../../build/kApi-gnumk_linux_x64-Debug/kThreadPool.d: kApi/Threads/kThreadPool.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kThreadPool.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kThreadPool.o -c kApi/Threads/kThreadPool.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kTimer.o ../../build/kApi-gnumk_linux_x64-Debug/kTimer.d: kApi/Threads/kTimer.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kTimer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kTimer.o -c kApi/Threads/kTimer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kSemaphore.o ../../build/kApi-gnumk_linux_x64-Debug/kSemaphore.d: kApi/Threads/kSemaphore.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kSemaphore.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kSemaphore.o -c kApi/Threads/kSemaphore.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kBackTrace.o ../../build/kApi-gnumk_linux_x64-Debug/kBackTrace.d: kApi/Utils/kBackTrace.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kBackTrace.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kBackTrace.o -c kApi/Utils/kBackTrace.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kDateTime.o ../../build/kApi-gnumk_linux_x64-Debug/kDateTime.d: kApi/Utils/kDateTime.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kDateTime.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kDateTime.o -c kApi/Utils/kDateTime.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kDebugAlloc.o ../../build/kApi-gnumk_linux_x64-Debug/kDebugAlloc.d: kApi/Utils/kDebugAlloc.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kDebugAlloc.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kDebugAlloc.o -c kApi/Utils/kDebugAlloc.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kDynamicLib.o ../../build/kApi-gnumk_linux_x64-Debug/kDynamicLib.d: kApi/Utils/kDynamicLib.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kDynamicLib.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kDynamicLib.o -c kApi/Utils/kDynamicLib.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kEvent.o ../../build/kApi-gnumk_linux_x64-Debug/kEvent.d: kApi/Utils/kEvent.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kEvent.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kEvent.o -c kApi/Utils/kEvent.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kObjectPool.o ../../build/kApi-gnumk_linux_x64-Debug/kObjectPool.d: kApi/Utils/kObjectPool.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kObjectPool.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kObjectPool.o -c kApi/Utils/kObjectPool.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kPlugin.o ../../build/kApi-gnumk_linux_x64-Debug/kPlugin.d: kApi/Utils/kPlugin.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kPlugin.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kPlugin.o -c kApi/Utils/kPlugin.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kPoolAlloc.o ../../build/kApi-gnumk_linux_x64-Debug/kPoolAlloc.d: kApi/Utils/kPoolAlloc.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kPoolAlloc.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kPoolAlloc.o -c kApi/Utils/kPoolAlloc.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kProcess.o ../../build/kApi-gnumk_linux_x64-Debug/kProcess.d: kApi/Utils/kProcess.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kProcess.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kProcess.o -c kApi/Utils/kProcess.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kSymbolInfo.o ../../build/kApi-gnumk_linux_x64-Debug/kSymbolInfo.d: kApi/Utils/kSymbolInfo.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kSymbolInfo.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kSymbolInfo.o -c kApi/Utils/kSymbolInfo.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kTimeSpan.o ../../build/kApi-gnumk_linux_x64-Debug/kTimeSpan.d: kApi/Utils/kTimeSpan.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kTimeSpan.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kTimeSpan.o -c kApi/Utils/kTimeSpan.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kUserAlloc.o ../../build/kApi-gnumk_linux_x64-Debug/kUserAlloc.d: kApi/Utils/kUserAlloc.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kUserAlloc.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kUserAlloc.o -c kApi/Utils/kUserAlloc.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Debug/kUtils.o ../../build/kApi-gnumk_linux_x64-Debug/kUtils.d: kApi/Utils/kUtils.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kUtils.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Debug/kUtils.o -c kApi/Utils/kUtils.cpp -MMD -MP

endif

ifeq ($(config),Release)

../../build/kApi-gnumk_linux_x64-Release/kAlloc.o ../../build/kApi-gnumk_linux_x64-Release/kAlloc.d: kApi/kAlloc.cpp
	$(SILENT) $(info GccX64 kApi/kAlloc.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kAlloc.o -c kApi/kAlloc.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kApiDef.o ../../build/kApi-gnumk_linux_x64-Release/kApiDef.d: kApi/kApiDef.cpp
	$(SILENT) $(info GccX64 kApi/kApiDef.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kApiDef.o -c kApi/kApiDef.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kApiLib.o ../../build/kApi-gnumk_linux_x64-Release/kApiLib.d: kApi/kApiLib.cpp
	$(SILENT) $(info GccX64 kApi/kApiLib.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kApiLib.o -c kApi/kApiLib.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kAssembly.o ../../build/kApi-gnumk_linux_x64-Release/kAssembly.d: kApi/kAssembly.cpp
	$(SILENT) $(info GccX64 kApi/kAssembly.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kAssembly.o -c kApi/kAssembly.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kObject.o ../../build/kApi-gnumk_linux_x64-Release/kObject.d: kApi/kObject.cpp
	$(SILENT) $(info GccX64 kApi/kObject.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kObject.o -c kApi/kObject.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kType.o ../../build/kApi-gnumk_linux_x64-Release/kType.d: kApi/kType.cpp
	$(SILENT) $(info GccX64 kApi/kType.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kType.o -c kApi/kType.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kValue.o ../../build/kApi-gnumk_linux_x64-Release/kValue.d: kApi/kValue.cpp
	$(SILENT) $(info GccX64 kApi/kValue.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kValue.o -c kApi/kValue.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kCipher.o ../../build/kApi-gnumk_linux_x64-Release/kCipher.d: kApi/Crypto/kCipher.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kCipher.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kCipher.o -c kApi/Crypto/kCipher.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kBlowfishCipher.o ../../build/kApi-gnumk_linux_x64-Release/kBlowfishCipher.d: kApi/Crypto/kBlowfishCipher.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kBlowfishCipher.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kBlowfishCipher.o -c kApi/Crypto/kBlowfishCipher.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kCipherStream.o ../../build/kApi-gnumk_linux_x64-Release/kCipherStream.d: kApi/Crypto/kCipherStream.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kCipherStream.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kCipherStream.o -c kApi/Crypto/kCipherStream.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kHash.o ../../build/kApi-gnumk_linux_x64-Release/kHash.d: kApi/Crypto/kHash.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kHash.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kHash.o -c kApi/Crypto/kHash.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kSha1Hash.o ../../build/kApi-gnumk_linux_x64-Release/kSha1Hash.d: kApi/Crypto/kSha1Hash.cpp
	$(SILENT) $(info GccX64 kApi/Crypto/kSha1Hash.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kSha1Hash.o -c kApi/Crypto/kSha1Hash.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kArray1.o ../../build/kApi-gnumk_linux_x64-Release/kArray1.d: kApi/Data/kArray1.cpp
	$(SILENT) $(info GccX64 kApi/Data/kArray1.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kArray1.o -c kApi/Data/kArray1.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kArray2.o ../../build/kApi-gnumk_linux_x64-Release/kArray2.d: kApi/Data/kArray2.cpp
	$(SILENT) $(info GccX64 kApi/Data/kArray2.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kArray2.o -c kApi/Data/kArray2.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kArray3.o ../../build/kApi-gnumk_linux_x64-Release/kArray3.d: kApi/Data/kArray3.cpp
	$(SILENT) $(info GccX64 kApi/Data/kArray3.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kArray3.o -c kApi/Data/kArray3.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kArrayList.o ../../build/kApi-gnumk_linux_x64-Release/kArrayList.d: kApi/Data/kArrayList.cpp
	$(SILENT) $(info GccX64 kApi/Data/kArrayList.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kArrayList.o -c kApi/Data/kArrayList.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kBox.o ../../build/kApi-gnumk_linux_x64-Release/kBox.d: kApi/Data/kBox.cpp
	$(SILENT) $(info GccX64 kApi/Data/kBox.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kBox.o -c kApi/Data/kBox.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kBytes.o ../../build/kApi-gnumk_linux_x64-Release/kBytes.d: kApi/Data/kBytes.cpp
	$(SILENT) $(info GccX64 kApi/Data/kBytes.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kBytes.o -c kApi/Data/kBytes.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kCollection.o ../../build/kApi-gnumk_linux_x64-Release/kCollection.d: kApi/Data/kCollection.cpp
	$(SILENT) $(info GccX64 kApi/Data/kCollection.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kCollection.o -c kApi/Data/kCollection.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kImage.o ../../build/kApi-gnumk_linux_x64-Release/kImage.d: kApi/Data/kImage.cpp
	$(SILENT) $(info GccX64 kApi/Data/kImage.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kImage.o -c kApi/Data/kImage.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kList.o ../../build/kApi-gnumk_linux_x64-Release/kList.d: kApi/Data/kList.cpp
	$(SILENT) $(info GccX64 kApi/Data/kList.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kList.o -c kApi/Data/kList.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kMath.o ../../build/kApi-gnumk_linux_x64-Release/kMath.d: kApi/Data/kMath.cpp
	$(SILENT) $(info GccX64 kApi/Data/kMath.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kMath.o -c kApi/Data/kMath.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kMap.o ../../build/kApi-gnumk_linux_x64-Release/kMap.d: kApi/Data/kMap.cpp
	$(SILENT) $(info GccX64 kApi/Data/kMap.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kMap.o -c kApi/Data/kMap.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kString.o ../../build/kApi-gnumk_linux_x64-Release/kString.d: kApi/Data/kString.cpp
	$(SILENT) $(info GccX64 kApi/Data/kString.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kString.o -c kApi/Data/kString.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kQueue.o ../../build/kApi-gnumk_linux_x64-Release/kQueue.d: kApi/Data/kQueue.cpp
	$(SILENT) $(info GccX64 kApi/Data/kQueue.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kQueue.o -c kApi/Data/kQueue.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kXml.o ../../build/kApi-gnumk_linux_x64-Release/kXml.d: kApi/Data/kXml.cpp
	$(SILENT) $(info GccX64 kApi/Data/kXml.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kXml.o -c kApi/Data/kXml.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kDat5Serializer.o ../../build/kApi-gnumk_linux_x64-Release/kDat5Serializer.d: kApi/Io/kDat5Serializer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kDat5Serializer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kDat5Serializer.o -c kApi/Io/kDat5Serializer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kDat6Serializer.o ../../build/kApi-gnumk_linux_x64-Release/kDat6Serializer.d: kApi/Io/kDat6Serializer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kDat6Serializer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kDat6Serializer.o -c kApi/Io/kDat6Serializer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kDirectory.o ../../build/kApi-gnumk_linux_x64-Release/kDirectory.d: kApi/Io/kDirectory.cpp
	$(SILENT) $(info GccX64 kApi/Io/kDirectory.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kDirectory.o -c kApi/Io/kDirectory.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kFile.o ../../build/kApi-gnumk_linux_x64-Release/kFile.d: kApi/Io/kFile.cpp
	$(SILENT) $(info GccX64 kApi/Io/kFile.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kFile.o -c kApi/Io/kFile.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kHttpServer.o ../../build/kApi-gnumk_linux_x64-Release/kHttpServer.d: kApi/Io/kHttpServer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kHttpServer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kHttpServer.o -c kApi/Io/kHttpServer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kHttpServerChannel.o ../../build/kApi-gnumk_linux_x64-Release/kHttpServerChannel.d: kApi/Io/kHttpServerChannel.cpp
	$(SILENT) $(info GccX64 kApi/Io/kHttpServerChannel.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kHttpServerChannel.o -c kApi/Io/kHttpServerChannel.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kHttpServerRequest.o ../../build/kApi-gnumk_linux_x64-Release/kHttpServerRequest.d: kApi/Io/kHttpServerRequest.cpp
	$(SILENT) $(info GccX64 kApi/Io/kHttpServerRequest.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kHttpServerRequest.o -c kApi/Io/kHttpServerRequest.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kHttpServerResponse.o ../../build/kApi-gnumk_linux_x64-Release/kHttpServerResponse.d: kApi/Io/kHttpServerResponse.cpp
	$(SILENT) $(info GccX64 kApi/Io/kHttpServerResponse.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kHttpServerResponse.o -c kApi/Io/kHttpServerResponse.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kMemory.o ../../build/kApi-gnumk_linux_x64-Release/kMemory.d: kApi/Io/kMemory.cpp
	$(SILENT) $(info GccX64 kApi/Io/kMemory.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kMemory.o -c kApi/Io/kMemory.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kNetwork.o ../../build/kApi-gnumk_linux_x64-Release/kNetwork.d: kApi/Io/kNetwork.cpp
	$(SILENT) $(info GccX64 kApi/Io/kNetwork.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kNetwork.o -c kApi/Io/kNetwork.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kPath.o ../../build/kApi-gnumk_linux_x64-Release/kPath.d: kApi/Io/kPath.cpp
	$(SILENT) $(info GccX64 kApi/Io/kPath.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kPath.o -c kApi/Io/kPath.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kPipeStream.o ../../build/kApi-gnumk_linux_x64-Release/kPipeStream.d: kApi/Io/kPipeStream.cpp
	$(SILENT) $(info GccX64 kApi/Io/kPipeStream.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kPipeStream.o -c kApi/Io/kPipeStream.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kSerializer.o ../../build/kApi-gnumk_linux_x64-Release/kSerializer.d: kApi/Io/kSerializer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kSerializer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kSerializer.o -c kApi/Io/kSerializer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kStream.o ../../build/kApi-gnumk_linux_x64-Release/kStream.d: kApi/Io/kStream.cpp
	$(SILENT) $(info GccX64 kApi/Io/kStream.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kStream.o -c kApi/Io/kStream.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kSocket.o ../../build/kApi-gnumk_linux_x64-Release/kSocket.d: kApi/Io/kSocket.cpp
	$(SILENT) $(info GccX64 kApi/Io/kSocket.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kSocket.o -c kApi/Io/kSocket.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kTcpClient.o ../../build/kApi-gnumk_linux_x64-Release/kTcpClient.d: kApi/Io/kTcpClient.cpp
	$(SILENT) $(info GccX64 kApi/Io/kTcpClient.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kTcpClient.o -c kApi/Io/kTcpClient.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kTcpServer.o ../../build/kApi-gnumk_linux_x64-Release/kTcpServer.d: kApi/Io/kTcpServer.cpp
	$(SILENT) $(info GccX64 kApi/Io/kTcpServer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kTcpServer.o -c kApi/Io/kTcpServer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kUdpClient.o ../../build/kApi-gnumk_linux_x64-Release/kUdpClient.d: kApi/Io/kUdpClient.cpp
	$(SILENT) $(info GccX64 kApi/Io/kUdpClient.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kUdpClient.o -c kApi/Io/kUdpClient.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kWebSocket.o ../../build/kApi-gnumk_linux_x64-Release/kWebSocket.d: kApi/Io/kWebSocket.cpp
	$(SILENT) $(info GccX64 kApi/Io/kWebSocket.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kWebSocket.o -c kApi/Io/kWebSocket.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kAtomic.o ../../build/kApi-gnumk_linux_x64-Release/kAtomic.d: kApi/Threads/kAtomic.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kAtomic.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kAtomic.o -c kApi/Threads/kAtomic.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kLock.o ../../build/kApi-gnumk_linux_x64-Release/kLock.d: kApi/Threads/kLock.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kLock.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kLock.o -c kApi/Threads/kLock.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kMsgQueue.o ../../build/kApi-gnumk_linux_x64-Release/kMsgQueue.d: kApi/Threads/kMsgQueue.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kMsgQueue.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kMsgQueue.o -c kApi/Threads/kMsgQueue.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kParallel.o ../../build/kApi-gnumk_linux_x64-Release/kParallel.d: kApi/Threads/kParallel.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kParallel.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kParallel.o -c kApi/Threads/kParallel.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kPeriodic.o ../../build/kApi-gnumk_linux_x64-Release/kPeriodic.d: kApi/Threads/kPeriodic.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kPeriodic.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kPeriodic.o -c kApi/Threads/kPeriodic.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kThread.o ../../build/kApi-gnumk_linux_x64-Release/kThread.d: kApi/Threads/kThread.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kThread.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kThread.o -c kApi/Threads/kThread.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kThreadPool.o ../../build/kApi-gnumk_linux_x64-Release/kThreadPool.d: kApi/Threads/kThreadPool.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kThreadPool.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kThreadPool.o -c kApi/Threads/kThreadPool.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kTimer.o ../../build/kApi-gnumk_linux_x64-Release/kTimer.d: kApi/Threads/kTimer.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kTimer.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kTimer.o -c kApi/Threads/kTimer.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kSemaphore.o ../../build/kApi-gnumk_linux_x64-Release/kSemaphore.d: kApi/Threads/kSemaphore.cpp
	$(SILENT) $(info GccX64 kApi/Threads/kSemaphore.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kSemaphore.o -c kApi/Threads/kSemaphore.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kBackTrace.o ../../build/kApi-gnumk_linux_x64-Release/kBackTrace.d: kApi/Utils/kBackTrace.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kBackTrace.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kBackTrace.o -c kApi/Utils/kBackTrace.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kDateTime.o ../../build/kApi-gnumk_linux_x64-Release/kDateTime.d: kApi/Utils/kDateTime.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kDateTime.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kDateTime.o -c kApi/Utils/kDateTime.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kDebugAlloc.o ../../build/kApi-gnumk_linux_x64-Release/kDebugAlloc.d: kApi/Utils/kDebugAlloc.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kDebugAlloc.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kDebugAlloc.o -c kApi/Utils/kDebugAlloc.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kDynamicLib.o ../../build/kApi-gnumk_linux_x64-Release/kDynamicLib.d: kApi/Utils/kDynamicLib.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kDynamicLib.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kDynamicLib.o -c kApi/Utils/kDynamicLib.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kEvent.o ../../build/kApi-gnumk_linux_x64-Release/kEvent.d: kApi/Utils/kEvent.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kEvent.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kEvent.o -c kApi/Utils/kEvent.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kObjectPool.o ../../build/kApi-gnumk_linux_x64-Release/kObjectPool.d: kApi/Utils/kObjectPool.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kObjectPool.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kObjectPool.o -c kApi/Utils/kObjectPool.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kPlugin.o ../../build/kApi-gnumk_linux_x64-Release/kPlugin.d: kApi/Utils/kPlugin.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kPlugin.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kPlugin.o -c kApi/Utils/kPlugin.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kPoolAlloc.o ../../build/kApi-gnumk_linux_x64-Release/kPoolAlloc.d: kApi/Utils/kPoolAlloc.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kPoolAlloc.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kPoolAlloc.o -c kApi/Utils/kPoolAlloc.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kProcess.o ../../build/kApi-gnumk_linux_x64-Release/kProcess.d: kApi/Utils/kProcess.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kProcess.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kProcess.o -c kApi/Utils/kProcess.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kSymbolInfo.o ../../build/kApi-gnumk_linux_x64-Release/kSymbolInfo.d: kApi/Utils/kSymbolInfo.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kSymbolInfo.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kSymbolInfo.o -c kApi/Utils/kSymbolInfo.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kTimeSpan.o ../../build/kApi-gnumk_linux_x64-Release/kTimeSpan.d: kApi/Utils/kTimeSpan.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kTimeSpan.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kTimeSpan.o -c kApi/Utils/kTimeSpan.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kUserAlloc.o ../../build/kApi-gnumk_linux_x64-Release/kUserAlloc.d: kApi/Utils/kUserAlloc.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kUserAlloc.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kUserAlloc.o -c kApi/Utils/kUserAlloc.cpp -MMD -MP

../../build/kApi-gnumk_linux_x64-Release/kUtils.o ../../build/kApi-gnumk_linux_x64-Release/kUtils.d: kApi/Utils/kUtils.cpp
	$(SILENT) $(info GccX64 kApi/Utils/kUtils.cpp)
	$(SILENT) $(CXX_COMPILER) $(COMPILER_FLAGS) $(CXX_FLAGS) $(DEFINES) $(INCLUDE_DIRS) -o ../../build/kApi-gnumk_linux_x64-Release/kUtils.o -c kApi/Utils/kUtils.cpp -MMD -MP

endif

ifeq ($(MAKECMDGOALS),all-obj)

ifeq ($(config),Debug)

include ../../build/kApi-gnumk_linux_x64-Debug/kAlloc.d
include ../../build/kApi-gnumk_linux_x64-Debug/kApiDef.d
include ../../build/kApi-gnumk_linux_x64-Debug/kApiLib.d
include ../../build/kApi-gnumk_linux_x64-Debug/kAssembly.d
include ../../build/kApi-gnumk_linux_x64-Debug/kObject.d
include ../../build/kApi-gnumk_linux_x64-Debug/kType.d
include ../../build/kApi-gnumk_linux_x64-Debug/kValue.d
include ../../build/kApi-gnumk_linux_x64-Debug/kCipher.d
include ../../build/kApi-gnumk_linux_x64-Debug/kBlowfishCipher.d
include ../../build/kApi-gnumk_linux_x64-Debug/kCipherStream.d
include ../../build/kApi-gnumk_linux_x64-Debug/kHash.d
include ../../build/kApi-gnumk_linux_x64-Debug/kSha1Hash.d
include ../../build/kApi-gnumk_linux_x64-Debug/kArray1.d
include ../../build/kApi-gnumk_linux_x64-Debug/kArray2.d
include ../../build/kApi-gnumk_linux_x64-Debug/kArray3.d
include ../../build/kApi-gnumk_linux_x64-Debug/kArrayList.d
include ../../build/kApi-gnumk_linux_x64-Debug/kBox.d
include ../../build/kApi-gnumk_linux_x64-Debug/kBytes.d
include ../../build/kApi-gnumk_linux_x64-Debug/kCollection.d
include ../../build/kApi-gnumk_linux_x64-Debug/kImage.d
include ../../build/kApi-gnumk_linux_x64-Debug/kList.d
include ../../build/kApi-gnumk_linux_x64-Debug/kMath.d
include ../../build/kApi-gnumk_linux_x64-Debug/kMap.d
include ../../build/kApi-gnumk_linux_x64-Debug/kString.d
include ../../build/kApi-gnumk_linux_x64-Debug/kQueue.d
include ../../build/kApi-gnumk_linux_x64-Debug/kXml.d
include ../../build/kApi-gnumk_linux_x64-Debug/kDat5Serializer.d
include ../../build/kApi-gnumk_linux_x64-Debug/kDat6Serializer.d
include ../../build/kApi-gnumk_linux_x64-Debug/kDirectory.d
include ../../build/kApi-gnumk_linux_x64-Debug/kFile.d
include ../../build/kApi-gnumk_linux_x64-Debug/kHttpServer.d
include ../../build/kApi-gnumk_linux_x64-Debug/kHttpServerChannel.d
include ../../build/kApi-gnumk_linux_x64-Debug/kHttpServerRequest.d
include ../../build/kApi-gnumk_linux_x64-Debug/kHttpServerResponse.d
include ../../build/kApi-gnumk_linux_x64-Debug/kMemory.d
include ../../build/kApi-gnumk_linux_x64-Debug/kNetwork.d
include ../../build/kApi-gnumk_linux_x64-Debug/kPath.d
include ../../build/kApi-gnumk_linux_x64-Debug/kPipeStream.d
include ../../build/kApi-gnumk_linux_x64-Debug/kSerializer.d
include ../../build/kApi-gnumk_linux_x64-Debug/kStream.d
include ../../build/kApi-gnumk_linux_x64-Debug/kSocket.d
include ../../build/kApi-gnumk_linux_x64-Debug/kTcpClient.d
include ../../build/kApi-gnumk_linux_x64-Debug/kTcpServer.d
include ../../build/kApi-gnumk_linux_x64-Debug/kUdpClient.d
include ../../build/kApi-gnumk_linux_x64-Debug/kWebSocket.d
include ../../build/kApi-gnumk_linux_x64-Debug/kAtomic.d
include ../../build/kApi-gnumk_linux_x64-Debug/kLock.d
include ../../build/kApi-gnumk_linux_x64-Debug/kMsgQueue.d
include ../../build/kApi-gnumk_linux_x64-Debug/kParallel.d
include ../../build/kApi-gnumk_linux_x64-Debug/kPeriodic.d
include ../../build/kApi-gnumk_linux_x64-Debug/kThread.d
include ../../build/kApi-gnumk_linux_x64-Debug/kThreadPool.d
include ../../build/kApi-gnumk_linux_x64-Debug/kTimer.d
include ../../build/kApi-gnumk_linux_x64-Debug/kSemaphore.d
include ../../build/kApi-gnumk_linux_x64-Debug/kBackTrace.d
include ../../build/kApi-gnumk_linux_x64-Debug/kDateTime.d
include ../../build/kApi-gnumk_linux_x64-Debug/kDebugAlloc.d
include ../../build/kApi-gnumk_linux_x64-Debug/kDynamicLib.d
include ../../build/kApi-gnumk_linux_x64-Debug/kEvent.d
include ../../build/kApi-gnumk_linux_x64-Debug/kObjectPool.d
include ../../build/kApi-gnumk_linux_x64-Debug/kPlugin.d
include ../../build/kApi-gnumk_linux_x64-Debug/kPoolAlloc.d
include ../../build/kApi-gnumk_linux_x64-Debug/kProcess.d
include ../../build/kApi-gnumk_linux_x64-Debug/kSymbolInfo.d
include ../../build/kApi-gnumk_linux_x64-Debug/kTimeSpan.d
include ../../build/kApi-gnumk_linux_x64-Debug/kUserAlloc.d
include ../../build/kApi-gnumk_linux_x64-Debug/kUtils.d

endif

ifeq ($(config),Release)

include ../../build/kApi-gnumk_linux_x64-Release/kAlloc.d
include ../../build/kApi-gnumk_linux_x64-Release/kApiDef.d
include ../../build/kApi-gnumk_linux_x64-Release/kApiLib.d
include ../../build/kApi-gnumk_linux_x64-Release/kAssembly.d
include ../../build/kApi-gnumk_linux_x64-Release/kObject.d
include ../../build/kApi-gnumk_linux_x64-Release/kType.d
include ../../build/kApi-gnumk_linux_x64-Release/kValue.d
include ../../build/kApi-gnumk_linux_x64-Release/kCipher.d
include ../../build/kApi-gnumk_linux_x64-Release/kBlowfishCipher.d
include ../../build/kApi-gnumk_linux_x64-Release/kCipherStream.d
include ../../build/kApi-gnumk_linux_x64-Release/kHash.d
include ../../build/kApi-gnumk_linux_x64-Release/kSha1Hash.d
include ../../build/kApi-gnumk_linux_x64-Release/kArray1.d
include ../../build/kApi-gnumk_linux_x64-Release/kArray2.d
include ../../build/kApi-gnumk_linux_x64-Release/kArray3.d
include ../../build/kApi-gnumk_linux_x64-Release/kArrayList.d
include ../../build/kApi-gnumk_linux_x64-Release/kBox.d
include ../../build/kApi-gnumk_linux_x64-Release/kBytes.d
include ../../build/kApi-gnumk_linux_x64-Release/kCollection.d
include ../../build/kApi-gnumk_linux_x64-Release/kImage.d
include ../../build/kApi-gnumk_linux_x64-Release/kList.d
include ../../build/kApi-gnumk_linux_x64-Release/kMath.d
include ../../build/kApi-gnumk_linux_x64-Release/kMap.d
include ../../build/kApi-gnumk_linux_x64-Release/kString.d
include ../../build/kApi-gnumk_linux_x64-Release/kQueue.d
include ../../build/kApi-gnumk_linux_x64-Release/kXml.d
include ../../build/kApi-gnumk_linux_x64-Release/kDat5Serializer.d
include ../../build/kApi-gnumk_linux_x64-Release/kDat6Serializer.d
include ../../build/kApi-gnumk_linux_x64-Release/kDirectory.d
include ../../build/kApi-gnumk_linux_x64-Release/kFile.d
include ../../build/kApi-gnumk_linux_x64-Release/kHttpServer.d
include ../../build/kApi-gnumk_linux_x64-Release/kHttpServerChannel.d
include ../../build/kApi-gnumk_linux_x64-Release/kHttpServerRequest.d
include ../../build/kApi-gnumk_linux_x64-Release/kHttpServerResponse.d
include ../../build/kApi-gnumk_linux_x64-Release/kMemory.d
include ../../build/kApi-gnumk_linux_x64-Release/kNetwork.d
include ../../build/kApi-gnumk_linux_x64-Release/kPath.d
include ../../build/kApi-gnumk_linux_x64-Release/kPipeStream.d
include ../../build/kApi-gnumk_linux_x64-Release/kSerializer.d
include ../../build/kApi-gnumk_linux_x64-Release/kStream.d
include ../../build/kApi-gnumk_linux_x64-Release/kSocket.d
include ../../build/kApi-gnumk_linux_x64-Release/kTcpClient.d
include ../../build/kApi-gnumk_linux_x64-Release/kTcpServer.d
include ../../build/kApi-gnumk_linux_x64-Release/kUdpClient.d
include ../../build/kApi-gnumk_linux_x64-Release/kWebSocket.d
include ../../build/kApi-gnumk_linux_x64-Release/kAtomic.d
include ../../build/kApi-gnumk_linux_x64-Release/kLock.d
include ../../build/kApi-gnumk_linux_x64-Release/kMsgQueue.d
include ../../build/kApi-gnumk_linux_x64-Release/kParallel.d
include ../../build/kApi-gnumk_linux_x64-Release/kPeriodic.d
include ../../build/kApi-gnumk_linux_x64-Release/kThread.d
include ../../build/kApi-gnumk_linux_x64-Release/kThreadPool.d
include ../../build/kApi-gnumk_linux_x64-Release/kTimer.d
include ../../build/kApi-gnumk_linux_x64-Release/kSemaphore.d
include ../../build/kApi-gnumk_linux_x64-Release/kBackTrace.d
include ../../build/kApi-gnumk_linux_x64-Release/kDateTime.d
include ../../build/kApi-gnumk_linux_x64-Release/kDebugAlloc.d
include ../../build/kApi-gnumk_linux_x64-Release/kDynamicLib.d
include ../../build/kApi-gnumk_linux_x64-Release/kEvent.d
include ../../build/kApi-gnumk_linux_x64-Release/kObjectPool.d
include ../../build/kApi-gnumk_linux_x64-Release/kPlugin.d
include ../../build/kApi-gnumk_linux_x64-Release/kPoolAlloc.d
include ../../build/kApi-gnumk_linux_x64-Release/kProcess.d
include ../../build/kApi-gnumk_linux_x64-Release/kSymbolInfo.d
include ../../build/kApi-gnumk_linux_x64-Release/kTimeSpan.d
include ../../build/kApi-gnumk_linux_x64-Release/kUserAlloc.d
include ../../build/kApi-gnumk_linux_x64-Release/kUtils.d

endif

endif

