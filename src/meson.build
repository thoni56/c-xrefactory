project('c-xrefactory', 'c',
        default_options: ['c_args = -Wall -Wno-char-subscripts -Werror -Wimplicit-fallthrough -funsigned-char'])

# We need the fs module
fs = import('fs')

all_modules = [ 'main', 'globals', 'misc', 'semact', 'commons',
  'complete', 'cxref', 'cxfile', 'lexer', 'lexembuffer', 'yylex',
  'options', 'caching', 'jsemact', 'classfilereader', 'classcaster',
  'init', 'editor', 'refactory', 'protocol', 'log', 'type', 'usage',
  'storage', 'extract', 'classhierarchy', 'jslsemact', 'filetable',
  'macroargumenttable', 'session', 'editorbuffertab', 'symboltable',
  'javafqttab', 'jsltypetab', 'reftab', 'memory', 'characterreader',
  'hash', 'symbol', 'cexp_parser.tab',
  'java_parser.tab', 'yacc_parser.tab', 'filedescriptor',
  'typemodifier', 'position', 'id', 'parsers', 'fileio', 'stringlist',
  'ppc', 'server', 'reference', 'xref', 'refactorings', 'progress',
  'input', 'commandlogger', 'completion', 'menu', 'lexem',
  'editorbuffer', 'undo', 'counters' ]

# List all modules and any extra objects they need when linking for unittesting
unittested_modules_with_dependencies = {
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
  'options': ['protocol', 'position', 'memory'],
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
cgreen_root = fs.parent(fs.parent(cgreen.full_path()))
cgreen_inc = include_directories(cgreen_root / 'include/cgreen')
cgreen_lib = meson.get_compiler('c').find_library('cgreen', dirs:cgreen_root / 'lib/')
cgreen_dep = declare_dependency(include_directories: cgreen_inc, dependencies: cgreen_lib)

# Compile and run tests for each module
test_libs = []
foreach module, deps : unittested_modules_with_dependencies
  test_filename = module + '_tests.c'
  # Ensure there is actually a test file
  if fs.is_file(test_filename)
    dep_files = []
    foreach dep : deps
      dep_files += dep + '.c'
    endforeach
    test_lib = shared_library(module + '_tests', [module + '.c', module + '_tests.c'] + dep_files + 'log.c',
                              dependencies: cgreen_dep)
  endif
  test_libs += test_lib
endforeach
unittests_target = custom_target('unittests', output: '.unittests_passed',
                                command: [cgreen, '-q', test_libs, '&&', 'touch', '@OUTPUT@'],
                                build_always_stale: true, build_by_default: true)

check_unittests_target = custom_target('check_unittests', output: '.unittests_dummy_output',
                                       command: [['test', '-f', unittests_target.full_path()]],
                                       build_always_stale: true,
                                       depends: unittests_target)

# Configure options_config.h with GIT hash/tag
git = find_program('git', required : true)

git_hash = run_command(git, 'describe', '--abbrev=5', '--always', '--tags', '--dirty', check:true).stdout().strip()
version = run_command(git, 'describe', '--abbrev=0', '--tags', check:true).stdout().strip()

conf_data = configuration_data()
conf_data.set('GIT_HASH', git_hash)
conf_data.set('VERSION', version)

configure_file(input: 'options_config.h.in',
               output: 'options_config.h',
               configuration: conf_data)

#
# Sources
#
sources = []
foreach module : all_modules
  sources += module + '.c'
endforeach

# Generate parsers
yacc = find_program('yacc')
c_parser_target = custom_target('c_parser',
  input: 'c_parser.y',
  output: ['c_parser.tab.c', 'c_parser.tab.h'],
  command: [yacc, '--defines=@OUTDIR@/c_parser.tab.h', '-o', '@OUTDIR@/c_parser.tab.c', '@INPUT@'],
  build_always_stale: true
)

# Generate lexem.h from the parser.tab.h and extra lexemes in character_lexems.txt
lexem_h = custom_target(output: 'lexem.h',
  input: ['character_lexems.txt', c_parser_target[1]],
  command: [
    'sh', '-c',
    '''
    echo Rebuilding lexem.h...
    echo "#ifndef LEXEM_H_INCLUDED" > @OUTPUT@.tmp
    echo "#define LEXEM_H_INCLUDED" >> @OUTPUT@.tmp
    echo >> @OUTPUT@.tmp
    echo "/**** DO NOT EDIT - generated from c_parser.tab.h & character_lexems.txt ****/" >> @OUTPUT@.tmp
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

lexem_dep = declare_dependency(
    sources: lexem_h  # ensure all sources consider the lexem_h as a prerequisite
)


executable('c-xref', sources, c_parser_target, check_unittests_target, dependencies: lexem_dep)
