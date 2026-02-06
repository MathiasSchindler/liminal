# Lexer

## Token Kinds
- Identifiers: `IDENT`
- Keywords: `KEYWORD` (case-insensitive match)
- Literals:
  - `INTEGER` (e.g., `42`)
  - `REAL` (e.g., `3.14`), note `1..5` yields `INTEGER`, `..`, `INTEGER`
  - `STRING` (`'text'`, escapes: `\\`, `\'`, `\"`, `\n`, `\r`, `\t`, `\xNN`)
  - `BYTES` (`b'\x00'`)
  - `CHAR` (string literal with length 1)
  - `FSTRING` (`f'hi {name}'` — lexed as a single token)
  - `DURATION` (`10ms`, `5s`, `2m`, `1h`)
  - `MONEY` (`$12.50`)
- Punctuation/Operators: `:`, `;`, `,`, `.`, `..`, `(`, `)`, `[`, `]`, `{`, `}`, `+`, `-`, `*`, `/`, `:=`, `=`, `<>`, `<`, `>`, `<=`, `>=`
- Word operators: `DIV`, `MOD`, `AND`, `OR`, `NOT`
- `EOF`, `ERROR`

## Keywords (case-insensitive)
```
program uses config types oracles var function begin end record schema describe matching
array of tuple integer real boolean char string byte bytes true false if then else case for to do in while repeat until loop parallel break continue return result try except on consult within budget stream retry yield wait attempts timeout from failure hint into with cost context create withsystem withmaxtokens onoverflow strategy
```

## Comments
- Line: `// ...`
- Block: `/* ... */` (non-nested)

## Spans
Each token carries `line` (1-based), `column` (1-based), `offset` (0-based), `length` (bytes).

## Errors
`TK_ERROR` emitted for unterminated strings, invalid money literals, unexpected characters. Message included in `Token.error_msg`.

## Tests
- Golden fixtures in `tests/fixtures/*.lim` with expected tokens `*.tokens`
- Run via `ctest` (target `liminal_lexer_tests`)

## Fuzzing
- Optional target `lexer_fuzz` (`ENABLE_FUZZING=ON`, Clang with libFuzzer preferred)
- Corpus-based (stdin) runner included for AFL++/libFuzzer workflows.
