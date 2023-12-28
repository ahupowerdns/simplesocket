project('simplesockets', 'cpp', default_options : ['cpp_std=c++17'])

fmt_dep = dependency('fmt', version: '>9', static: true)

executable('testrunner', 'test.cc', 'sclasses.cc', 'swrappers.cc', 'comboaddress.cc',
	dependencies: [fmt_dep])



simplesockets_lib = library(
  'simplesockets',
  'comboaddress.cc', 'swrappers.cc', 'sclasses.cc',
  install: false,
  include_directories: '',
  dependencies: [fmt_dep]
)

simplesockets_dep = declare_dependency(
  link_with: simplesockets_lib,
  include_directories: '',
)



if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('simplesockets', simplesockets_dep)
endif
