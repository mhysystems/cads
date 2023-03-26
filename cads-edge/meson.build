project('cads', ['c','cpp'],
  version : '1.0',
  default_options : ['warning_level=3'])

message('Host system type: ' + host_machine.system())

add_project_arguments(
#  '-Wextra',
#  '-Wconversion',
#  '-Wsign-conversion', #too many warnings related to size_t
#  '-Wnull-dereference',
#  '-Wuseless-cast',
#  '-Wdouble-promotion',
#  '-Wshadow',
#  '-Wlogical-op',
#  '-Wduplicated-cond',
#  '-Wduplicated-branches',
  language : 'cpp')
#add_project_arguments('-w',language : 'cpp')


thread = dependency('threads')
boost = dependency('boost', modules : ['program_options'])
sqlite = dependency('sqlite3')
opencv = dependency('opencv4', required:false, include_type:'system')
tbb = dependency('tbb')
#gsl = dependency('gsl')

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
eigen_dep = subproject('eigen').get_variable('eigen_dep')
natsc_dep = subproject('nats.c').get_variable('natsc_dep')
brotli_enc_dep = subproject('google-brotli').get_variable('brotli_encoder_dep')

flatc_exe = subproject('flatbuffers').get_variable('flatc')
copy = find_program('cp')
run_command(copy,flatc_exe.full_path(),join_paths(meson.source_root(),'..','bin','flatc'))
run_command(copy,join_paths(flatc_exe.full_path(),'libflatbuffers.so'),join_paths(meson.source_root(),'..','bin','libflatbuffers.so'))

subdir('GO_SDK/Platform/kApi')
subdir('GO_SDK/Gocator/GoSdk')

cadsInc = include_directories('include')
cadsSrc = []
cadsEntry = []

libdep = [opencv, tbb, thread, boost, sqlite, http_stuff,log_stuff, flatbuffers, fmt_dep, nlohmann_json_dep,date_dep,tz_dep,iir1_dep,lua_dep,eigen_dep, natsc_dep, brotli_enc_dep]
# Returns z_config 
# Represents header files
subdir('flatbuffers')

subdir('src')
#subdir('test')



cads = executable(
	'cads', 
	cadsSrc + cadsEntry,
	include_directories : [GoSdkInc,kApiInc,cadsInc],
	dependencies:libdep,
  link_with : [GoSdkLib,kApiLib],
  #link_args : ['-fsanitize=address'], # GCC Address Sanitzer
  #cpp_args : ['-std=c++20','-fcoroutines', '-fsanitize=address'] # GCC Address Sanitzer
  cpp_args : ['-std=c++20','-fcoroutines'] 
)
