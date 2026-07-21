project('cppbound', 'cpp', version: '1.0.0',
        default_options: ['cpp_std=c++20'],
        meson_version: '>=0.54'
       )

include_dirs = include_directories('.')
headers = files('cobi.hpp', 'cobarray.hpp', 'coblist.hpp', 'cobfwdlist.hpp', 'cobskiplist.hpp')

install_headers(headers, subdir: 'cppbound')

cppbound_dep = declare_dependency(include_directories: include_dirs)
meson.override_dependency('cppbound', cppbound_dep)

test('cppbound_test', executable('bound.t', ['bound.t.cpp'], dependencies: [cppbound_dep]))
test('coblist_test', executable('coblist.t', ['coblist.t.cpp'], dependencies: [cppbound_dep]))
test('cobfwdlist_test', executable('cobfwdlist.t', ['cobfwdlist.t.cpp'], dependencies: [cppbound_dep]))
test('cobskiplist_test', executable('cobskiplist.t', ['cobskiplist.t.cpp'], dependencies: [cppbound_dep]))
test('cobtimers_test', executable('cobtimers.t', ['cobtimers.t.cpp'], dependencies: [cppbound_dep]))
#test('nonullptr_test', executable('nonullptr.t', ['nonullptr.t.cpp'], dependencies: [cppbound_dep]))
test('perhaps_test', executable('perhaps.t', ['perhaps.t.cpp'], dependencies: [cppbound_dep]))
