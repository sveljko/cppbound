project('cppbound', 'cpp', version: '1.0.0',
        default_options: ['cpp_std=c++20'],
        meson_version: '>=0.54'
       )

include_dirs = include_directories('.')
headers = files(
  'cobi.hpp', 'cobhlp.hpp', 'cobstore.hpp', 'cobarray.hpp', 'cobmatrix.hpp',
  'coblist.hpp', 'cobfwdlist.hpp', 'cobskiplist.hpp',
  'cobtmrlist.hpp', 'cobtmrwheel.hpp', 'cobtmrmill.hpp', 'cobtmrleanmill.hpp',
  'perhaps.hpp', 'fullptr.hpp', 'optptr.hpp', 'contingent.hpp', 'atbdim.hpp', 'atbarray.hpp',
  'cobpool.hpp', 'cobring.hpp', 'cobbitset.hpp', 'cobhash.hpp',
  'fullshared.hpp', 'optshared.hpp', 'fullunique.hpp', 'optunique.hpp',
  'bndarray.hpp', 'cobbound.hpp', 'cobchain.hpp', 'cobmap.hpp', 'cobvector.hpp', 'cobbinheap.hpp', 'cobstring.hpp',
  'atbbound.hpp', 'atbpool.hpp', 'atblist.hpp', 'atbfwdlist.hpp', 'atbskiplist.hpp', 'atbmap.hpp',
  'atbvector.hpp', 'atbbinheap.hpp', 'atbstring.hpp', 'atbring.hpp', 'atbbitset.hpp', 'atbhash.hpp',
  'cobtree.hpp', 'cobtreemap.hpp', 'cobspsc.hpp', 'cobfunction.hpp', 'coblru.hpp',
  'atbtree.hpp', 'atbtreemap.hpp', 'atbspsc.hpp', 'atbfunction.hpp', 'atblru.hpp',
  'cobflatset.hpp', 'cobflatmap.hpp', 'atbflatset.hpp', 'atbflatmap.hpp',
  'atbmatrix.hpp', 'atbtmrlist.hpp', 'atbtmrwheel.hpp', 'atbtmrmill.hpp', 'atbtmrleanmill.hpp',
)

install_headers(headers, subdir: 'cppbound')

cppbound_dep = declare_dependency(include_directories: include_dirs)
meson.override_dependency('cppbound', cppbound_dep)

test('cppbound_test', executable('bound.t', ['bound.t.cpp'], dependencies: [cppbound_dep]))
test('coblist_test', executable('coblist.t', ['coblist.t.cpp'], dependencies: [cppbound_dep]))
test('cobfwdlist_test', executable('cobfwdlist.t', ['cobfwdlist.t.cpp'], dependencies: [cppbound_dep]))
test('cobskiplist_test', executable('cobskiplist.t', ['cobskiplist.t.cpp'], dependencies: [cppbound_dep]))
test('cobtimers_test', executable('cobtimers.t', ['cobtimers.t.cpp'], dependencies: [cppbound_dep]))
test('fullptr_test', executable('fullptr.t', ['fullptr.t.cpp'], dependencies: [cppbound_dep]))
test('optptr_test', executable('optptr.t', ['optptr.t.cpp'], dependencies: [cppbound_dep]))
test('contingent_test', executable('contingent.t', ['contingent.t.cpp'], dependencies: [cppbound_dep]))
test('fullshared_test', executable('fullshared.t', ['fullshared.t.cpp'], dependencies: [cppbound_dep]))
test('optshared_test', executable('optshared.t', ['optshared.t.cpp'], dependencies: [cppbound_dep]))
test('fullunique_test', executable('fullunique.t', ['fullunique.t.cpp'], dependencies: [cppbound_dep]))
test('optunique_test', executable('optunique.t', ['optunique.t.cpp'], dependencies: [cppbound_dep]))
test('atbarray_test', executable('atbarray.t', ['atbarray.t.cpp'], dependencies: [cppbound_dep]))
test('cobpool_test', executable('cobpool.t', ['cobpool.t.cpp'], dependencies: [cppbound_dep]))
test('cobring_test', executable('cobring.t', ['cobring.t.cpp'], dependencies: [cppbound_dep]))
test('cobbitset_test', executable('cobbitset.t', ['cobbitset.t.cpp'], dependencies: [cppbound_dep]))
test('cobhash_test', executable('cobhash.t', ['cobhash.t.cpp'], dependencies: [cppbound_dep]))
test('perhaps_test', executable('perhaps.t', ['perhaps.t.cpp'], dependencies: [cppbound_dep]))
test('cobmap_test', executable('cobmap.t', ['cobmap.t.cpp'], dependencies: [cppbound_dep]))
test('cobvector_test', executable('cobvector.t', ['cobvector.t.cpp'], dependencies: [cppbound_dep]))
test('cobbinheap_test', executable('cobbinheap.t', ['cobbinheap.t.cpp'], dependencies: [cppbound_dep]))
test('cobstring_test', executable('cobstring.t', ['cobstring.t.cpp'], dependencies: [cppbound_dep]))
test('atbcontainers_test', executable('atbcontainers.t', ['atbcontainers.t.cpp'], dependencies: [cppbound_dep]))
test('checked_test', executable('checked.t', ['checked.t.cpp'], dependencies: [cppbound_dep]))
test('cobflat_test', executable('cobflat.t', ['cobflat.t.cpp'], dependencies: [cppbound_dep]))
test('hive_test', executable('hive.t', ['hive.t.cpp'], dependencies: [cppbound_dep]))
test('corners_test', executable('corners.t', ['corners.t.cpp'], dependencies: [cppbound_dep]))
test('docs_test', executable('docs.t', ['docs.t.cpp'], dependencies: [cppbound_dep]))
test('cobtree_test', executable('cobtree.t', ['cobtree.t.cpp'], dependencies: [cppbound_dep]))
test('cobfunction_test', executable('cobfunction.t', ['cobfunction.t.cpp'], dependencies: [cppbound_dep]))
test('coblru_test', executable('coblru.t', ['coblru.t.cpp'], dependencies: [cppbound_dep]))
test('cobspsc_test', executable('cobspsc.t', ['cobspsc.t.cpp'], dependencies: [cppbound_dep, dependency('threads')]))

# What must not compile, is checked not to. As that takes a compiler run for
# each case, it's not done when we're a subproject.
if not meson.is_subproject()
  cpp = meson.get_compiler('cpp')
  nocompile = files('nocompile.t.cpp')
  nocompile_args = ['-I' + meson.current_source_dir()]
  if not cpp.compiles(nocompile, args: nocompile_args, name: 'nocompile.t.cpp as is')
    error('nocompile.t.cpp should compile when no case is selected')
  endif
  foreach i : range(1, 86)
    if cpp.compiles(nocompile, args: nocompile_args + ['-DNOCOMPILE=@0@'.format(i)],
                    name: 'nocompile.t.cpp case @0@'.format(i))
      error('nocompile.t.cpp case @0@ should not compile'.format(i))
    endif
  endforeach
endif
