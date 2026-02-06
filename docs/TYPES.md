# Types & Symbol Tables

## Types (semantic)
- Primitive singletons: `Integer`, `Real`, `Boolean`, `String`, `Bytes`, `Char`, `Byte`, `Unknown`
- Composite:
  - `array(T)`
  - `tuple<T1,...,Tn>`
  - `alias(name -> T)`

## Symbol Table
- Scopes stack with lookup (case-insensitive)
- Symbols: `VAR`, `TYPE`, `FUNC` (function typing not implemented yet)

## Type Checking (Milestone 4 scope)
- Resolves types and vars (globals); built-ins recognized
- Expressions: literals, identifiers, unary/binary, tuple, array, index
- Assignments: type match required
- Errors collected with spans

## Tests
- `tests/test_typecheck.c`
  - `typecheck_ok` (numeric ops)
  - `type_mismatch` (string into integer)
  - `undeclared` (use before declare)

## Notes
- Simplified type system for now; no functions/oracles/checking of schemas yet
- Primitive types are singletons (no free needed)
