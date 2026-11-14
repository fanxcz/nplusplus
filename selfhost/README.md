# N++ self-hosting

This directory grows the compiler toolchain written in N++ itself. The C++
compiler in `src/` remains the reference implementation; components here are
mirrored against it feature by feature and must not diverge.

## Current components

| Component | File | Status |
|---|---|---|
| Lexer | `lexer_lib.npp` | Works; covers the full token set of `src/compiler/frontend/lexer/lexer.cpp` |
| Lexer tests | `lexer_test.npp` | 19 cases: keywords, decimal/radix numbers, operators, escapes, nested comments, and lexical errors |
| Self-lexing smoke | `lex_self.npp` | The N++ lexer tokenizes its own sources plus real stdlib/example files |
| Parser (subset) | `parser_lib.npp` | Works; recursive descent with correct precedence for expressions plus `let`/assignment/`print`/`if`/`else-if`/`else`/`while`/blocks/`fn`/`return` |
| Parser tests | `parser_test.npp` | 14 cases: precedence, associativity, calls, trailing commas, else-if chains, blocks, 3 error cases |
| Type checker (subset) | `typecheck_lib.npp` | Works; variable environment, numeric/string/bool rules, bool conditions for `if`/`while`, assignment type stability |
| Type checker tests | `typecheck_test.npp` | 13 cases: 3 valid programs, 10 diagnostics |
| Code generator (subset) | `codegen_lib.npp` | Works; emits a complete LLVM IR module (alloca/load/store SSA, branches, loops) for the i64/f64 statement subset, plus function declarations/calls, string literals and f64 arithmetic/print |
| Code generator tests | `codegen_test.npp` | End-to-end: N++ compiles `tests/hello_subset.npp` and `tests/fn_subset.npp` to `.ll`, clang++ builds them, the binaries run and print the expected outputs |

`tokenize(src: string) -> string` emits one token per line as
`LINE:COL KIND TEXT` (`Integer 42`, `KwFn fn`, `String decoded-text`, ...).
On the first lexical error it stops and emits `error LINE:COL MESSAGE`,
matching the reference lexer's failure points: unterminated strings, unknown
escapes, unexpected characters, unterminated block comments.

The lexer self-lexes cleanly: `npp run selfhost/lex_self.npp --backend llvm`
tokenizes `lexer_lib.npp` (~1800 tokens), `lex_self.npp` itself and several
stdlib/example sources with zero errors. `lexer_test.npp` is excluded from
self-lexing on purpose because its source embeds invalid inputs for the
negative test cases.

## Tests

`ctest -R selfhost` runs both programs through the LLVM backend and fails on
any non-zero exit or unexpected output. The test needs a clang-compatible
`NPP_CXX`; plain g++ cannot link the `.ll` pipeline.

## Roadmap

1. ~~Lexer~~ (done)
2. ~~Parser for the statement/expression subset~~ (done; `fn` declarations, structs, enums, `match`, `for` and typed annotations remain)
2b. Type checking for the parsed subset, verified against the C++ parser's
   acceptance/rejection on the same inputs
3. Symbol table and type checking for the subset the lexer covers
4. Code generation through the existing LLVM IR text pipeline — the N++
   compiler would then emit `.ll` for simple programs
5. Only after all of the above: attempt to compile `lexer_lib.npp` with the
   N++-written compiler. Self-hosting is declared only when that binary
   reproduces the token stream of the C++ pipeline on the whole test corpus.

## Constraints found while writing N++-in-N++

- No dynamically sized arrays: token streams are accumulated as strings
  instead. A `Vec`-style builtin or growable array type is the single most
  useful language extension for the parser stage.
- No string character type: all scanning is done with `string.byte_at` and
  byte comparisons, which works but makes the code verbose.
- Structs cannot be passed by reference into helper functions, so the lexer
  is one function rather than a state object plus helpers.
