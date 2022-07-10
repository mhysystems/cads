cads_unit_tests = executable(
  'cads_unit_tests', 
  sources : cadsSrc + [ 
    'cads_unit_tests.cpp'
  ],
  include_directories : [GoSdkInc,kApiInc,cadsInc],
  dependencies : [gtest_dep, gtest_main] + libdep,
  link_with : [GoSdkLib,kApiLib],
  cpp_args : ['-std=c++20','-fcoroutines', '-fext-numeric-literals']
  )

custom_target('copy script',
  input : 'tconfig.json',
  output :  'tconfig.json',
  command : ['cp', '@INPUT@', '@OUTPUT@'],
  install : false,
  build_by_default : true)


test('nan_removal', cads_unit_tests, args : ['--gtest_filter=cads.nan_filter'])
test('spike_filter', cads_unit_tests, args : ['--gtest_filter=cads.spike_filter'])
test('upload_belt', cads_unit_tests, args : ['--gtest_filter=cads.upload_belt'])
test('find_profile_edges_nans_outer', cads_unit_tests, args : ['--gtest_filter=cads.find_profile_edges_nans_outer'])
