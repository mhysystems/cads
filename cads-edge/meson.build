project('cads', ['c','cpp'],
  version : '1.0',
  default_options : ['warning_level=0'])

add_project_arguments('-w',language : 'cpp')

thread = dependency('threads')
boost = dependency('boost', modules : ['program_options','math_tr1f'])
sqlite = dependency('sqlite3')
opencv = dependency('opencv4', required:false)
tbb = dependency('tbb')

if not opencv.found()
	opencv = dependency('opencv')
endif

http_stuff = subproject('cpr').get_variable('cpr_dep')
log_stuff = subproject('spdlog').get_variable('spdlog_dep')
flatbuffers = subproject('flatbuffers').get_variable('flatbuffers_dep')
flatc = subproject('flatbuffers').get_variable('flatc')
gtest_dep = subproject('gtest').get_variable('gtest_dep')
gtest_main = subproject('gtest').get_variable('gtest_main_dep')
fmt_dep = subproject('fmt').get_variable('fmt_dep')

subdir('GO_SDK/Platform/kApi')
subdir('GO_SDK/Gocator/GoSdk')

cadsInc = include_directories('include')
cadsSrc = []
subdir('src')
subdir('test')

# Returns z_config and p_config
# Represents header files
subdir('flatbuffers')

cads = executable(
	'cads', 
	cadsSrc,
	include_directories : [GoSdkInc,kApiInc,cadsInc],
	dependencies:[opencv, tbb, thread, boost, sqlite, http_stuff,log_stuff, flatbuffers, fmt_dep],
  link_with : [GoSdkLib,kApiLib],
	cpp_args : ['-std=c++20','-fcoroutines']
)
