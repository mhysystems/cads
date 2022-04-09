nan_removal_tests = executable(
  'nan_removal_tests', 
  sources : [ 'nan_removal.cpp', meson.source_root() + '/src/nan_removal.cpp'],
  include_directories : cadsInc, 
  dependencies : [ gtest_dep, gtest_main ],
  cpp_args : ['-std=c++20','-fcoroutines']
  )


test('nan_removal', nan_removal_tests)