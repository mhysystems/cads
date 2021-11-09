kApiSrc = [
	'kApi/kAlloc.cpp',
	'kApi/kType.cpp',
	'kApi/Io/kDat5Serializer.cpp',
	'kApi/Io/kSocket.cpp',
	'kApi/Io/kHttpServerChannel.cpp',
	'kApi/Io/kDirectory.cpp',
	'kApi/Io/kTcpClient.cpp',
	'kApi/Io/kNetwork.cpp',
	'kApi/Io/kPath.cpp',
	'kApi/Io/kFile.cpp',
	'kApi/Io/kStream.cpp',
	'kApi/Io/kHttpServerRequest.cpp',
	'kApi/Io/kHttpServerResponse.cpp',
	'kApi/Io/kWebSocket.cpp',
	'kApi/Io/kPipeStream.cpp',
	'kApi/Io/kMemory.cpp',
	'kApi/Io/kUdpClient.cpp',
	'kApi/Io/kSerializer.cpp',
	'kApi/Io/kTcpServer.cpp',
	'kApi/Io/kHttpServer.cpp',
	'kApi/Io/kDat6Serializer.cpp',
	'kApi/kValue.cpp',
	'kApi/Utils/kDebugAlloc.cpp',
	'kApi/Utils/kObjectPool.cpp',
	'kApi/Utils/kPoolAlloc.cpp',
	'kApi/Utils/kPlugin.cpp',
	'kApi/Utils/kDateTime.cpp',
	'kApi/Utils/kUserAlloc.cpp',
	'kApi/Utils/kUtils.cpp',
	'kApi/Utils/kBackTrace.cpp',
	'kApi/Utils/kTimeSpan.cpp',
	'kApi/Utils/kSymbolInfo.cpp',
	'kApi/Utils/kEvent.cpp',
	'kApi/Utils/kProcess.cpp',
	'kApi/Utils/kDynamicLib.cpp',
	'kApi/Crypto/kCipherStream.cpp',
	'kApi/Crypto/kBlowfishCipher.cpp',
	'kApi/Crypto/kHash.cpp',
	'kApi/Crypto/kSha1Hash.cpp',
	'kApi/Crypto/kCipher.cpp',
	'kApi/kAssembly.cpp',
	'kApi/kApiLib.cpp',
	'kApi/kApiDef.cpp',
	'kApi/Data/kArray3.cpp',
	'kApi/Data/kArray1.cpp',
	'kApi/Data/kImage.cpp',
	'kApi/Data/kList.cpp',
	'kApi/Data/kXml.cpp',
	'kApi/Data/kMath.cpp',
	'kApi/Data/kMap.cpp',
	'kApi/Data/kBytes.cpp',
	'kApi/Data/kArray2.cpp',
	'kApi/Data/kArrayList.cpp',
	'kApi/Data/kCollection.cpp',
	'kApi/Data/kQueue.cpp',
	'kApi/Data/kString.cpp',
	'kApi/Data/kBox.cpp',
	'kApi/Threads/kTimer.cpp',
	'kApi/Threads/kThread.cpp',
	'kApi/Threads/kSemaphore.cpp',
	'kApi/Threads/kLock.cpp',
	'kApi/Threads/kThreadPool.cpp',
	'kApi/Threads/kPeriodic.cpp',
	'kApi/Threads/kMsgQueue.cpp',
	'kApi/Threads/kAtomic.cpp',
	'kApi/Threads/kParallel.cpp',
	'kApi/kObject.cpp'
]

ccpp = meson.get_compiler('cpp')
ccpp_args = ccpp.get_supported_arguments([
	'-Wno-unused-variable',
	'-Wno-unused-parameter',
	'-Wno-unused-value',
	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-cast-function-type'
]) + ['-DK_EMIT', '-DK_PLUGIN']

ccpp_dep = [
	thread,
	ccpp.find_library('dl', required : false),
	ccpp.find_library('m', required : false),
	ccpp.find_library('rt', required : false)
]

if get_option('debug')
	ccpp_args += ['-DK_DEBUG']
endif

kApiInc = include_directories('.')

kApiLib = library(
	'kApi',
  kApiSrc,
	cpp_args: ccpp_args,
	dependencies:ccpp_dep,
  install : true
)