project('cppbound', 'cpp', version: '1.0.0')

include_dirs = include_directories('.')
headers = files('cobi.hpp', 'cobarray.hpp')

install_headers(headers, subdir: 'cppbound')

cppbound_dep = declare_dependency(include_directories: include_dirs)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('cppbound', cppbound_dep)
endif

test('cppbound_test', executable('cppbound_test', ['bound.t.cpp'], dependencies: [cppbound_dep]))
