# Parser & AST

## AST
- Nodes: `Program`, `Config`, `TypeDecl`, `OracleDecl`, `VarDecl`, `FuncDecl`
- Expressions: literals, identifiers, unary/binary ops, call, index, slice, field, tuple, array, record, `ask`, `embed`
- Statements: assign, if/while/for/repeat/case/loop/parallel, break/continue, return (`Result := expr`), block, expr statement
- Types: primitive/ident, array, tuple, record, enum, optional, result, constrained, schema
- Spans: every node carries `LiminalSpan` (line/column/offset/length)

## Parser
- Recursive descent + Pratt for expressions
- Top-level: `program ...;` + optional blocks (`uses`, `config`, `types`, `oracles`, `var`, functions) + `begin ... end.`
- Handles `ask ... <- expr [else expr]`
- Error collection with spans

## Tests
- Fixtures: `tests/fixtures/parser_{basic,exprs,errors}.lim` + `.ast` snapshots
- Parser test: `liminal_parser_tests`
- AST printer: `ast_print` used for golden snapshots

## Notes
- `ask`/`embed` parsed with simple grammar (oracle ident, `<-` tokens as `<` `-`)
- Fuzzing not yet added for parser (future)
