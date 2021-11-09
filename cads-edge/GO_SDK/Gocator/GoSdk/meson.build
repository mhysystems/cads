GoSdkSrc = [
'GoSdk/GoPartMatching.c',
'GoSdk/GoReplay.c',
'GoSdk/GoAlgorithm.c',
'GoSdk/GoPartModel.c',
'GoSdk/Internal/GoDiscovery.c',
'GoSdk/Internal/GoReceiver.c',
'GoSdk/Internal/GoAccelSensorPortAlloc.c',
'GoSdk/Internal/GoControl.c',
'GoSdk/Internal/GoSerializer.c',
'GoSdk/GoSetup.c',
'GoSdk/GoProfileGeneration.c',
'GoSdk/GoSdkDef.c',
'GoSdk/Outputs/GoEthernet.c',
'GoSdk/Outputs/GoOutput.c',
'GoSdk/Outputs/GoDigital.c',
'GoSdk/Outputs/GoAnalog.c',
'GoSdk/Outputs/GoSerial.c',
'GoSdk/GoUtils.c',
'GoSdk/GoAcceleratorMgr.c',
'GoSdk/GoRecordingFilter.c',
'GoSdk/GoSensorInfo.c',
'GoSdk/GoSurfaceGeneration.c',
'GoSdk/GoMultiplexBank.c',
'GoSdk/Messages/GoDataSet.c',
'GoSdk/Messages/GoDiscoveryExtInfo.c',
'GoSdk/Messages/GoDataTypes.c',
'GoSdk/Messages/GoHealth.c',
'GoSdk/GoSensor.c',
'GoSdk/GoAccelerator.c',
'GoSdk/GoLayout.c',
'GoSdk/GoReplayCondition.c',
'GoSdk/GoPartDetection.c',
'GoSdk/GoAdvanced.c',
'GoSdk/GoGeoCal.c',
'GoSdk/GoSections.c',
'GoSdk/GoSdkLib.c',
'GoSdk/GoTracheid.c',
'GoSdk/GoMaterial.c',
'GoSdk/GoTransform.c',
'GoSdk/Tools/GoMeasurement.c',
'GoSdk/Tools/GoExtToolDataOutput.c',
'GoSdk/Tools/GoSurfaceToolUtils.c',
'GoSdk/Tools/GoExtTool.c',
'GoSdk/Tools/GoExtParam.c',
'GoSdk/Tools/GoMeasurements.c',
'GoSdk/Tools/GoTools.c',
'GoSdk/Tools/GoTool.c',
'GoSdk/Tools/GoFeatures.c',
'GoSdk/Tools/GoRangeTools.c',
'GoSdk/Tools/GoProfileToolUtils.c',
'GoSdk/Tools/GoExtParams.c',
'GoSdk/Tools/GoFeature.c',
'GoSdk/Tools/GoExtMeasurement.c',
'GoSdk/Tools/GoProfileTools.c',
'GoSdk/Tools/GoSurfaceTools.c',
'GoSdk/GoSection.c',
'GoSdk/GoSystem.c'
]

cc = meson.get_compiler('c')
cc_args = cc.get_supported_arguments([
	'-Wno-unused-variable',
	'-Wno-unused-parameter',
	'-Wno-unused-value',
	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-sign-compare',
	'-Wno-pedantic',
	'-Wno-unused-but-set-parameter',
	'-Wno-cast-function-type'
]) + ['-DK_EMIT', '-DEXPERIMENTAL_FEATURES_ENABLED']


cc_dep = [
	thread,
	cc.find_library('dl', required : false),
	cc.find_library('m', required : false),
	cc.find_library('rt', required : false)
]

if get_option('debug')
	cc_args += ['-DK_DEBUG']
endif

GoSdkInc = include_directories('.')


GoSdkLib = library(
	'GoSdk',
	GoSdkSrc,
	include_directories : kApiInc,
	c_args : cc_args,
	dependencies : cc_dep,
	link_with : [kApiLib],
	install : true
)