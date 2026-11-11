# pasc

A compiler for ISO/IEC 7185:1990 Pascal, written in C++17, targeting x86-64
Linux (System V ABI). This is the first vertical slice: a small subset that
runs end to end, with the structure the rest of the standard will grow into.

## Build and test

    cmake -S . -B build && cmake --build build && ctest --test-dir build

or, without CMake:

    make test

The CMake Debug build compiles the compiler with ASan and UBSan.
Requirements: a C++17 compiler, a C compiler reachable as `cc`, GNU binutils.

## Use

    build/pasc tests/pass/count.pas -o count && ./count
    build/pasc -S -o count.s tests/pass/count.pas     # assembly only
    build/pasc --dump-tokens file.pas
    build/pasc --dump-ast file.pas                    # typed AST and frame layout

Exit status: 0 success, 1 the program was rejected, 2 a driver failure.

## Pipeline

    lexer -> parser -> sema -> codegen_x86 -> cc (assemble, link libpasrt.a)

| File            | Role                                                     |
|-----------------|----------------------------------------------------------|
| lexer           | all of 6.1, including the alternative tokens of 6.1.9    |
| parser          | recursive descent, unresolved AST, no name lookup        |
| sema            | scopes (6.2.2), types (6.4), checks (6.7, 6.8), frames   |
| codegen_x86     | naive codegen: results in rax, temporaries pushed        |
| runtime/rt.c    | write procedures and run-time error reports              |

The backend boundary is the typed AST. Sema resolves every name and type, so
a second backend (LLVM IR) only needs another consumer of `Program`.

## Implemented

- Lexical: complete (real numbers are lexed, then rejected by the parser)
- Program heading and program parameters, with the 6.10 checks
- `var` declarations of `integer`, `Boolean`, `char`
- Statements: assignment, compound, `if`, `while`, `repeat`, empty
- Expressions: `+ - * div mod`, signs, relational operators, `and or not`
- `write` and `writeln` to `output`, with field widths
- Run-time errors: overflow, `div` by zero, `mod` by a non-positive value,
  field width below one

## Not implemented yet

Each of these is refused with a "not implemented yet" diagnostic.

1. `const` and `type` parts, subranges, enumerations
2. `for`, `case`, `goto` and labels
3. Procedures and functions, static links, parameters
4. Arrays, records, sets, pointers, `with`
5. The real-type
6. Files, `read`, `readln`, the required functions
7. Conformant arrays (level 1)

See docs/LEDGER.md for the implementation-defined choices and the status of
every error the standard lets a processor leave undetected.

## Tests

`tests/run.sh` runs three directories, which mirror how conformance suites
are organised:

- `pass/`    must compile, run, and print exactly `NAME.expected`
- `reject/`  must be refused at compile time
- `runtime/` must compile, then stop with a reported run-time error

Add a file to the right directory for every new feature.
