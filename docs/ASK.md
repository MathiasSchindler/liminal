# ask and Result Types

## Semantics
- `ask Oracle <- Expr` returns a **Result** value: `Ok(text)` on success, `Err(message)` on failure.
- Fallback: `ask Oracle <- Expr else FallbackExpr` returns `Ok(Fallback)` when the oracle fails (fallback evaluated eagerly).
- `UnwrapOr(default)` works on Results: returns the contained text when `Ok`, otherwise `default`.

## Runtime Representation
- `VRESULT`: `{ ok: bool, text: string?, error: string? }`
- Printing:
  - `Ok(text)` prints `Ok(<text>)`
  - `Err(msg)` prints `Err(<msg>)`

## Parser Support
- `ask Oracle <- expr [else expr]`
- `result.UnwrapOr(expr)` recognized as a builtin method

> `timeout`, `into`, `with cost` are reserved; not implemented yet.

## Tests
- `liminal_ask_tests`
  - `ask_ok`: succeeds via mock
  - `ask_else`: fallback on failure
  - `ask_unwrapor`: `UnwrapOr` returns fallback
  - `ask_err`: Err without else
  - `ask_chain`: fallback ask
- `liminal_parser_tests`: `parser_ask` AST fixture
- `liminal_ir_tests`: `ir_ask` snapshot
- `liminal_cli_tests`: `cli_ask_else`

## Notes
- Fallback is evaluated eagerly (simplified for Milestone 10).
- `timeout`, `into`, `with cost` parsed but not yet enforced; cost tracking in Milestone 11.
