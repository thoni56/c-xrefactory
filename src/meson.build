project('c-xrefactory-unittest', 'c')

# We need the fs module
fs = import('fs')

# List all modules and any extra objects they need when linking for unittesting
modules_with_unittest_dependencies = {
  'caching': ['memory'],
  'characterreader': [],
  'classfilereader': ['memory', 'position'],
  'classhierarchy': ['memory', 'protocol'],
  'commandlogger': [],
  'commons': ['protocol', 'stringlist'],
  'complete': ['memory', 'protocol', 'type', 'session'],
  'completion': ['memory', 'usage', 'session'],
  'counters': [],
  'cxfile': ['memory', 'usage', 'protocol', 'position', 'hash', 'session'],
  'cxref': ['server', 'protocol', 'hash', 'position', 'type', 'storage', 'usage', 'session', 'memory'],
  'editor': ['position', 'usage', 'memory', 'hash'],
  'editorbuffer': ['position', 'memory'],
  'editorbuffertab': ['position', 'memory', 'hash'],
  'extract': ['position', 'memory', 'storage', 'type', 'id', 'usage', 'protocol'],
  'fileio': [],
  'filetable': ['memory', 'hash'],
  'id': ['memory',],
  'init': [],
  'jsemact': ['usage', 'memory', 'position', 'symbol', 'id'],
  'jslsemact': ['memory', 'symbol', 'id'],
  'lexembuffer': ['characterreader', 'position', 'memory'],
  'lexer': ['lexembuffer', 'characterreader', 'position', 'memory'],
  'macroargumenttable': ['memory', 'hash'],
  'main': ['memory', 'protocol', 'position', 'memory', 'session'],
  'memory': [],
  'menu': ['protocol', 'position', 'memory', 'usage'],
  'misc': ['protocol', 'type', 'memory'],
   #'options': ['protocol', 'position', 'memory'],
  'position': ['memory'],
  'progress': ['protocol'],
  'refactorings': [],
  'refactory': ['protocol', 'usage', 'position', 'memory', 'session'],
  'reftab': [ 'memory', 'hash'],
  'semact': ['memory', 'protocol', 'usage', 'hash', 'position', 'storage', 'type', 'id'],
  'server': ['memory', 'session', 'position'],
  'symbol': ['memory'],
  'symboltable': ['hash', 'memory'],
  'undo': ['memory'],
  'xref': ['memory', 'protocol'],
  'yylex': ['memory', 'protocol', 'position', 'filedescriptor', 'id', 'filetable', 'hash', 'symboltable', 'macroargumenttable', 'symbol', 'lexembuffer', 'stringlist']
}

#
# Unittests
#
cgreen = find_program('cgreen-runner')
cgreen_inc = include_directories('/usr/include/cgreen')
cgreen_lib = meson.get_compiler('c').find_library('cgreen', dirs:'/usr/lib/')
cgreen_dep = declare_dependency(include_directories: cgreen_inc, dependencies: cgreen_lib)

# Compile and run tests for each module
test_libs = []
foreach mod, deps : modules_with_unittest_dependencies
  test_filename = mod + '_tests.c'
  if fs.is_file(test_filename)
    dep_files = []
    foreach dep : deps
      dep_files += dep + '.c'
    endforeach
    test_lib = shared_library(mod + '_tests', [mod + '.c', mod + '_tests.c'] + dep_files + 'log.c',
                              dependencies: cgreen_dep)
  endif
  test_libs += test_lib
endforeach
custom_target('unittests', output: '.unittests_dummy_output', command: [cgreen, '-q', test_libs], build_always_stale: true, build_by_default: true)

#
# Sources
#
sources = []
foreach mod, _ : modules_with_unittest_dependencies
  sources += mod + '.c'
endforeach

#
# Compile each module
#
foreach mod, _ : modules_with_unittest_dependencies
  # This can be your normal build process for the module itself
  # For example, you might compile each module into an object file
  # which you later link into your main executable.
endforeach

#
# Link c-xref
#
executable('c-xref', sources)
