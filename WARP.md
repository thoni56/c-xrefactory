# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Project Overview

c-xrefactory is a free Emacs refactoring tool and code browser for C & Yacc. It provides advanced code navigation, symbol browsing, and automated refactoring capabilities through a client-server architecture where the c-xref server (written in C) communicates with Emacs via a custom protocol.

## Build System & Common Commands

### Production Build
```bash
make prod              # Build optimized c-xref for production use
```

### Development Build & Testing
```bash
make                   # Build with coverage and run unit tests
make all               # Same as above - build and run unit tests
make unit              # Build and run unit tests with coverage
make devel             # Build, run quick tests, produce coverage
make ci                # Full CI build - unit tests + system tests + coverage
```

### Testing
```bash
make test              # Run all tests (from top level)
make -C tests quick    # Run only fast tests
make -C tests slow     # Run only slow tests  
make -C tests all      # Run both fast and slow tests
make -C tests verbose  # Run tests with detailed output
```

### Coverage & Analysis
```bash
make coverage          # Generate .gcov files for Emacs cov-mode
make clean-coverage    # Clean coverage data
make coverage-report   # Generate HTML coverage report
```

### Individual Test Execution
To run a single test (useful for debugging):
```bash
./tests/run_test tests/test_<testname>
```

### Build Artifacts Cleanup
```bash
make clean             # Clean build artifacts
make -C src clean      # Clean source directory
make -C byacc-1.9 clean # Clean bundled yacc
```

## Architecture & Key Components

### Core Architecture
- **Client-Server Model**: Emacs client communicates with c-xref server via custom protocol
- **Parser Pipeline**: Uses bundled yacc-1.9 to generate C and Yacc parsers
- **Integrated Preprocessor**: Custom C preprocessor implementation that preserves macro information for navigation and refactoring (does not use system cpp)
- **Memory Management**: Custom memory allocation with stack-based memory for performance
- **Symbol Database**: Maintains cross-reference database for navigation and refactoring

### Critical Source Directories
- `src/`: Core C implementation (~389 files)
  - `main.c`: Entry point for c-xref server
  - `server.c`: Server communication handling
  - `*_parser.y`: Yacc grammar files for C and Yacc parsing
  - `lexer.c`, `yylex.c`: Lexical analysis
  - `semact.c`: Semantic actions during parsing
  - `refactory.c`: Core refactoring logic
  - `complete.c`: Code completion engine
  - `cxref.c`: Cross-reference generation
  - Memory subsystem: `memory.c`, `stackmemory.c`
  - Symbol management: `symbol.c`, `symboltable.c`, `reference.c`

### Test Infrastructure
- `tests/`: Comprehensive test suite with 140+ test directories
- Each test directory contains:
  - Input C/Yacc files
  - Expected output files
  - Test-specific configuration
- `tests/run_test`: Test execution script with coverage collection
- Test categories: unit tests (fast), system tests (slow), integration tests

### Editor Integration
- `editors/emacs/`: Emacs Lisp integration
  - `c-xrefactory.el`: Main Emacs interface
  - `c-xref.el`: Core functionality binding
  - `c-xrefprotocol.el`: Protocol communication

## Development Workflow

### Automated Testing
**IMPORTANT**: This project uses file watches that automatically rebuild and run tests when code changes are detected. **Do NOT run `make` or test commands unless explicitly requested by the user.** Simply make code changes and wait for the user to report test results from the watches.

### Parser Regeneration
When modifying grammar files:
```bash
# Yacc files automatically regenerate .tab.c files via Makefile rules
make -C src clean    # Force regeneration of parsers
```

### Adding New Tests
1. Create directory: `tests/test_<feature_name>/`
2. Add test files and expected outputs
3. Create `.slow` file if test is slow
4. Tests auto-discovered by Makefile

### Debugging
The build system supports debugging flags:
```bash
make EXTRA_CFLAGS=-DYYDEBUG build  # Enable parser debugging
```

### Coverage Analysis
The project uses gcov/lcov for coverage:
- Unit tests: ~80% coverage achieved
- Coverage files: `.gcov` (for Emacs cov-mode), `.info` (for lcov)
- HTML reports generated in `coverage/` directory

## Key Dependencies

### Build Dependencies
- C compiler (gcc/clang)
- yacc (bundled yacc-1.9 included)
- libz, libcjson
- On macOS: Homebrew libraries auto-detected

### Runtime Dependencies  
- Emacs (for client interface)
- Standard Unix tools: make, bash

## Special Considerations

### macOS Development
- Auto-detects Homebrew paths for ARM64 vs x86_64
- Can use gcc-14 if available
- Uses compatible gcov tool based on compiler

### Yacc Grammar Limitations
- Uses custom yacc-1.9 (not modern bison)
- Grammars not adapted to modern yacc/bison
- Java support removed (grammar was Java 1.4 era)

### Memory Architecture
- Custom memory management for performance
- Stack-based allocation for temporary data during parsing
- Extensive use of generated hash tables and lists

### LSP Experimental Support
Basic LSP server functionality exists but is experimental:
- `lsp_*.c` files contain LSP protocol handlers
- Communication works but features not yet accessible
- Future development direction for IDE integration

## Refactoring Capabilities

The tool supports sophisticated refactoring operations:
- Symbol renaming (variables, functions, types, macros)
- Function/macro parameter manipulation (add, delete, reorder)
- Code extraction (functions, macros, variables) 
- Include file renaming
- Dead code detection and removal
- Yacc grammar symbol navigation