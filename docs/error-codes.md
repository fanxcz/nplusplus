# Error and warning codes

N++ diagnostics carry a stable `NX####` code so scripts, the LSP and CI can
match on them without parsing message text. Codes are grouped by pipeline
stage; the numeric range identifies the stage.

## Code ranges

| Range | Stage | Emitted by |
| --- | --- | --- |
| `NX0xxx` | Driver / uncategorized | CLI, project loader, tools that have no source position |
| `NX1xxx` | Frontend | Lexer and parser |
| `NX2xxx` | Semantic analysis | Type checker, name resolution, borrow rules |
| `NX9xxx` | Lint / quality | `npp lint` and related quality checks |

## Reference

| Code | Severity | Meaning | Typical fix |
| --- | --- | --- | --- |
| `NX0000` | Error | Generic driver error without a finer classification (file not found, bad CLI usage rendered through the diagnostic path, project/manifest problems) | Read the message; it names the failing path or option |
| `NX1000` | Error | Lexer failure — the source could not be tokenized | Fix the offending characters; check for stray control characters or unterminated string/comment delimiters |
| `NX1001` | Error | Syntax error — tokens do not form a valid program | Check the reported line/column; parser recovery can report several independent errors in one run |
| `NX2000` | Error | Semantic error summary header produced by the analyzer (`N++ found N semantic errors`) | Each listed `- ` line is an individual semantic failure; fix them bottom-up |
| `NX2001` | Error | Individual semantic error — type mismatch, unknown variable/field, borrow violation, non-exhaustive match, immutable assignment, and similar | Read the span caret; the message names the symbol and expected type |
| `NX9001` | Warning | `npp lint`: trailing spaces or tabs at end of a source line | Remove trailing whitespace (CRLF line endings are ignored) |
| `NX9002` | Warning | `npp lint`: a line contains `TODO` or `FIXME` | Resolve the marker or track the work in an issue |

## Notes

- Semantic analysis aggregates independent errors across declarations and
  function bodies, so one `npp check` run can surface many `NX2001` entries.
- The LSP maps `NX1000`/`NX1001`/`NX2001` onto editor diagnostics; `NX2001`
  entries additionally drive "unknown variable" quick-fix suggestions.
- Backend rejections (e.g. a construct outside the builtin native subset) are
  reported as ordinary semantic/codegen messages, not `NX` codes. Run
  `npp explain-backend file.npp` for the native-vs-LLVM decision.
