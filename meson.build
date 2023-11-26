project('cppbound', 'cpp', version: '1.0.0', default_options: ['cpp_std=c++17'])

include_dirs = include_directories('.')
headers = files('cobi.hpp', 'cobarray.hpp', 'coblist.hpp', 'cobfwdlist.hpp', 'cobskiplist.hpp')

install_headers(headers, subdir: 'cppbound')

cppbound_dep = declare_dependency(include_directories: include_dirs)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('cppbound', cppbound_dep)
endif

test('cppbound_test', executable('cppbound_test', ['bound.t.cpp'], dependencies: [cppbound_dep]))
test('coblist_test', executable('coblist.t', ['coblist.t.cpp'], dependencies: [cppbound_dep]))
test('cobfwdlist_test', executable('cobfwdlist.t', ['cobfwdlist.t.cpp'], dependencies: [cppbound_dep]))
test('cobskiplist_test', executable('cobskiplist.t', ['cobskiplist.t.cpp'], dependencies: [cppbound_dep]))
