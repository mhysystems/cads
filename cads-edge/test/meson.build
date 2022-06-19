cads_unit_tests = executable(
  'cads_unit_tests', 
  sources : [ 
    'cads_unit_tests.cpp', 
    meson.source_root() + '/src/nan_removal.cpp',  
    meson.source_root() + '/src/fiducial.cpp',
    meson.source_root() + '/src/db.cpp',
    meson.source_root() + '/src/window.cpp',
    meson.source_root() + '/src/filters.cpp',
    meson.source_root() + '/src/constants.cpp',
    meson.source_root() + '/src/edge_detection.cpp',
    meson.source_root() + '/src/dynamic_processing.cpp'
  ],
  include_directories : [GoSdkInc,kApiInc,cadsInc],
  dependencies : [opencv, gtest_dep, gtest_main, fmt_dep, log_stuff, sqlite, nlohmann_json_dep, iir1_dep, lua_dep ],
  cpp_args : ['-std=c++20','-fcoroutines', '-fext-numeric-literals']
  )


test('nan_removal', cads_unit_tests, args : ['--gtest_filter=cads.nan_removal'])
test('make_fiducial', cads_unit_tests, args : ['--gtest_filter=cads.make_fiducial'])
test('search_for_fiducial', cads_unit_tests, args : ['--gtest_filter=cads.search_for_fiducial'])
test('spike_filter', cads_unit_tests, args : ['--gtest_filter=cads.spike_filter','-c','tconfig.json'])
test('dynamic_processing', cads_unit_tests, args : ['--gtest_filter=cads.dynamic_processing'])
test('find_profile_edges_nans_outer', cads_unit_tests, args : ['--gtest_filter=cads.find_profile_edges_nans_outer'])