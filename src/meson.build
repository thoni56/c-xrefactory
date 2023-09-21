project('c-xrefactory', 'c')

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
cgreen_inc = include_directories('/usr/local/include/cgreen')
cgreen_lib = meson.get_compiler('c').find_library('cgreen', dirs:'/usr/local/lib/')
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
# Modules
#
modules = []
foreach module, _ : modules_with_unittest_dependencies
  modules += module + '.c'
endforeach

#
# Compile each module
#
foreach mod, _ : modules_with_unittest_dependencies
  # This can be your normal build process for the module itself
  # For example, you might compile each module into an object file
  # which you later link into your main executable.
endforeach


# Automatically find version and hash from git
git_hash = run_command('git', 'describe', '--abbrev=5', '--always', '--tags', '--dirty', check:true).stdout().strip()
git_tag = run_command('git', 'describe', '--abbrev=0', '--tags', check:true).stdout().strip()

compiled_git_hash = '-DGIT_HASH="@0@"'.format(git_hash)
compiled_git_tag = '-DVERSION="@0@"'.format(git_tag)

# Apply the GIT flags to 'options.c'
if 'options.c' in sources
  c_args = [compiled_git_hash, compiled_git_tag]
else
  c_args = []
endif

# Generate lexem.h from the parser.tab.h and extra lexemes in character_lexems.txt
custom_target('generate_lexem_h',
  output: 'lexem.h',
  input: ['character_lexems.txt', 'c_parser.tab.h'],
  command: [
    'sh', '-c',
    '''
    echo Rebuilding lexem.h...
    echo "#ifndef LEXEM_H_INCLUDED" > @OUTPUT@.tmp
    echo "#define LEXEM_H_INCLUDED" >> @OUTPUT@.tmp
	echo >> @OUTPUT@.tmp
	echo "/**** DO NOT EDIT - generated from c_parser.tab.h & character_lexems.txt by Makefile ****/" >> @OUTPUT@.tmp
	echo >> @OUTPUT@.tmp
    cat @0@ >> @OUTPUT@.tmp
    grep define @1@ | sed -e "s/#define \\(.*\\) \\([0-9]*\\)/\\1 = \\2,/" >> @OUTPUT@.tmp
	echo "typedef enum LexemCode {" >> @OUTPUT@.tmp
    echo >> @OUTPUT@.tmp
	echo "extern void initLexemEnumNames(void);" >> @OUTPUT@.tmp
	echo >> @OUTPUT@.tmp
	echo "#endif" >> @OUTPUT@.tmp
	# Sanity check for content, otherwise yacc errored out probably
    if grep " = " @OUTPUT@.tmp > /dev/null; then
      mv @OUTPUT@.tmp @OUTPUT@
    else
      rm @OUTPUT@.tmp
    fi
    ''',
    'character_lexems.txt', 'c_parser.tab.h'
  ],
  build_always_stale: true
)

executable('c-xref', sources, 'lexem.h', c_args: c_args, include_directories: '.')
