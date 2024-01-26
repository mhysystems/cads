project('cads', ['c','cpp'],
  version : '1.0',
  default_options : ['warning_level=3','cpp_std=c++23','default_library=static','prefer_static=true'])

message('Host system type: ' + host_machine.system())

add_project_arguments(
  '-Wextra',
#  '-Wconversion',
#  '-Wsign-conversion', #too many warnings related to size_t
  '-Wnull-dereference',
#  '-Wuseless-cast',
#  '-Wdouble-promotion',
#  '-Wshadow',
#  '-Wlogical-op',
#  '-Wduplicated-cond',
#  '-Wduplicated-branches',
  language : 'cpp')

cxx = meson.get_compiler('cpp')
add_project_arguments(cxx.get_supported_arguments('-march=native'), language: 'cpp')
add_project_arguments(cxx.get_supported_arguments('-mno-outline-atomics'), language: 'cpp')
add_project_arguments(cxx.get_supported_arguments('-fconcepts-diagnostics-depth=4'), language: 'cpp')

curl = dependency('libcurl', static : false)
thread = dependency('threads', static : false)
boost = dependency('boost', modules : ['program_options'], static : false)
sqlite = dependency('sqlite3', static : false)
opencv = dependency('opencv4', required:false, include_type:'system', static : false)
tbb = dependency('tbb', static : false)
gsl = dependency('gsl', static : false)


if not opencv.found()
	opencv = dependency('opencv',include_type:'system')
endif

http_stuff = subproject('cprcads',default_options : ['warning_level=0','tests=disabled']).get_variable('cpr_dep')
log_stuff = subproject('spdlog',default_options : ['tests=false']).get_variable('spdlog_dep')
flatbuffers = subproject('flatbuffers').get_variable('flatbuffers_dep')
flatc = subproject('flatbuffers').get_variable('flatc')
#gtest_dep = subproject('gtest').get_variable('gtest_dep')
#gtest_main = subproject('gtest').get_variable('gtest_main_dep')
fmt_dep = subproject('fmt').get_variable('fmt_dep')
date_dep = subproject('hinnant-date', default_options : ['warning_level=0','use_system_tzdb=true']).get_variable('date_dep')
tz_dep = subproject('hinnant-date', default_options : ['use_system_tzdb=enabled']).get_variable('tz_dep')
nlohmann_json_dep = subproject('nlohmann_json').get_variable('nlohmann_json_dep')
iir1_dep = subproject('iir', default_options : ['default_library=static','prefer_static=true']).get_variable('libiir_dep')
lua_dep = subproject('lua').get_variable('lua_dep')
eigen_dep = subproject('eigen').get_variable('eigen_dep')
natsc_dep = subproject('nats.c').get_variable('natsc_dep')
brotli_enc_dep = subproject('google-brotli').get_variable('brotli_encoder_dep')
scamp_dep = subproject('scamp',default_options : ['default_library = both']).get_variable('scamp_dep')
scamp_file = subproject('scamp',default_options : ['default_library = both']).get_variable('scamplib')

flatc_exe = subproject('flatbuffers').get_variable('flatc')
copy = find_program('cp')
fs = import('fs')
run_command(copy,flatc_exe.full_path(),join_paths(meson.source_root(),'..','bin','flatc'))
run_command(copy,join_paths(fs.parent(flatc_exe.full_path()),'libflatbuffers.so'),join_paths(meson.source_root(),'..','bin','libflatbuffers.so'))
run_command(copy,scamp_file.full_path(),join_paths(meson.source_root(),'..','bin',fs.name(scamp_file.full_path())))

subdir('GO_SDK/Platform/kApi')
subdir('GO_SDK/Gocator/GoSdk')

cadsInc = include_directories('include')
cadsSrc = []
cadsEntry = []

libdep = [curl, opencv, tbb, thread, boost, sqlite, http_stuff,log_stuff, flatbuffers, fmt_dep, nlohmann_json_dep,date_dep,tz_dep,iir1_dep,lua_dep,eigen_dep, natsc_dep, brotli_enc_dep, gsl, scamp_dep]
# Returns z_config 
# Represents header files
subdir('flatbuffers')

subdir('src')
#subdir('test')


fs.copyfile('json.lua')

linkArgs = []
# scamp uses the cuda compiler and for some reason meson uses nvcc to link the whole program because of this.
# Without using the nvcc argument ccbin, nvcc will use link against an old versions of standards libs, even though the
# program is compiled with later versions of gcc. Note compiling cuda with new versions of gcc returns an error but linking
# is ok.
if add_languages('cuda', required: false)
  linkArgs += ['-ccbin=' + find_program(cxx.cmd_array()[0]).full_path()] 
endif

cads = executable(
	'cads', 
	cadsSrc + cadsEntry,
	include_directories : [GoSdkInc,kApiInc,cadsInc],
	dependencies:libdep,
  link_with : [GoSdkLib,kApiLib],
  link_args : [linkArgs],
  #link_args : ['-fsanitize=address'], # GCC Address Sanitzer
  #cpp_args : ['-fcoroutines', '-fsanitize=address'] # GCC Address Sanitzer
  cpp_args : ['-fcoroutines'] 
)
