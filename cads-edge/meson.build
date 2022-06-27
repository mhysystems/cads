project('cads', ['c','cpp'],
  version : '1.0',
  default_options : ['warning_level=3'])

add_project_arguments('-Wextra',
  '-Wconversion',
#  '-Wsign-conversion', #too many warnings related to size_t
  '-Wnull-dereference',
  '-Wuseless-cast',
  '-Wdouble-promotion',
  '-Wshadow',
  '-Wlogical-op',
  '-Wduplicated-cond',
  '-Wduplicated-branches',
  language : 'cpp')
#add_project_arguments('-w',language : 'cpp')


thread = dependency('threads')
boost = dependency('boost', modules : ['program_options','math_tr1f'])
sqlite = dependency('sqlite3')
opencv = dependency('opencv4', required:false, include_type:'system')
tbb = dependency('tbb')

if not opencv.found()
	opencv = dependency('opencv',include_type:'system')
endif

http_stuff = subproject('cpr',default_options : ['warning_level=0']).get_variable('cpr_dep')
log_stuff = subproject('spdlog').get_variable('spdlog_dep')
flatbuffers = subproject('flatbuffers').get_variable('flatbuffers_dep')
flatc = subproject('flatbuffers').get_variable('flatc')
gtest_dep = subproject('gtest').get_variable('gtest_dep')
gtest_main = subproject('gtest').get_variable('gtest_main_dep')
fmt_dep = subproject('fmt').get_variable('fmt_dep')
date_dep = subproject('hinnant-date', default_options : ['warning_level=0']).get_variable('date_dep')
tz_dep = subproject('hinnant-date').get_variable('tz_dep')
nlohmann_json_dep = subproject('nlohmann_json').get_variable('nlohmann_json_dep')
iir1_dep = subproject('iir1').get_variable('iir1_dep')
lua_dep = subproject('lua').get_variable('lua_dep')

subdir('GO_SDK/Platform/kApi')
subdir('GO_SDK/Gocator/GoSdk')

cadsInc = include_directories('include')
cadsSrc = []
subdir('src')
subdir('test')

# Returns z_config 
# Represents header files
subdir('flatbuffers')

cads = executable(
	'cads', 
	cadsSrc,
	include_directories : [GoSdkInc,kApiInc,cadsInc],
	dependencies:[opencv, tbb, thread, boost, sqlite, http_stuff,log_stuff, flatbuffers, fmt_dep, nlohmann_json_dep,date_dep,tz_dep,iir1_dep,lua_dep],
  link_with : [GoSdkLib,kApiLib],
#  link_args : ['-fsanitize=address'],
	cpp_args : ['-std=c++20','-fcoroutines'] #,'-fsanitize=address']
)
