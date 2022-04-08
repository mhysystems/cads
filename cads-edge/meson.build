project('cads', ['c','cpp'],
  version : '1.0',
  default_options : ['warning_level=0'])

add_project_arguments('-w',language : 'cpp')

thread = dependency('threads')
boost = dependency('boost', modules : ['program_options','math_tr1f'])
sqlite = dependency('sqlite3')
opencv = dependency('opencv', required:false)
flatbuffers = dependency('flatbuffers')

if not opencv.found()
	opencv = dependency('opencv4')
endif

http_stuff = subproject('cpr').get_variable('cpr_dep')
log_stuff = subproject('spdlog').get_variable('spdlog_dep')

subdir('GO_SDK/Platform/kApi')
subdir('GO_SDK/Gocator/GoSdk')

cadsInc = include_directories('include')
cadsSrc = []
subdir('src')

# Returns z_config and p_config
# Represents header files
subdir('flatbuffers')

cads = executable(
	'cads', 
	cadsSrc,
	include_directories : [GoSdkInc,kApiInc,cadsInc],
	dependencies:[opencv,thread, boost, sqlite, http_stuff,log_stuff, flatbuffers],
  link_with : [GoSdkLib,kApiLib],
	cpp_args : ['--std=c++20','-fcoroutines']
)
