project('nats.c', ['c','cpp'],
  version : '3.3.0',
  default_options : ['warning_level=3'])

protobufc = dependency('libprotobuf-c')

sourceFiles = []
includeDirectories = []
subdir('src')

libnatsc = static_library('nats.c', sourceFiles, dependencies : [protobufc], include_directories: includeDirectories)
natsc_dep = declare_dependency(include_directories: includeDirectories, link_with: libnatsc)