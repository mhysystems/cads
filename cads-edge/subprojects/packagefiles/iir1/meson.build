project('iir1', 'cpp',
    version : 'head',
    license : 'MIT'
)

src = files(
  'iir/Biquad.cpp',
  'iir/Butterworth.cpp',
  'iir/Cascade.cpp',
  'iir/ChebyshevI.cpp',
  'iir/ChebyshevII.cpp',
  'iir/Custom.cpp',
  'iir/PoleFilter.cpp',
  'iir/RBJ.cpp',
)

includes = include_directories('.')

iir1_lib = static_library('iir1', src,include_directories: includes)
iir1_dep = declare_dependency(include_directories: includes, link_with: iir1_lib)