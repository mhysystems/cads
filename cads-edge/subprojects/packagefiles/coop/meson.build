project('coop', 'cpp', 
    version : 'head', 
    license : 'MIT'
)

includes = include_directories('include')
coopSrc = []
subdir('src')

coop_lib = static_library('coop', coopSrc,include_directories: includes,  cpp_args : ['-std=c++20','-fcoroutines'])
coop_dep = declare_dependency(include_directories: includes, link_with: coop_lib)
