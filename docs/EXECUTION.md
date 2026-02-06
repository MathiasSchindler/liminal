# Deterministic Execution

## Overview
Milestone 8 adds a deterministic interpreter over the IR to run programs end-to-end (no oracles).

## Pipeline
1. Parse source → AST
2. Typecheck
3. Lower to IR (`ir_from_ast`)
4. Validate (`ir_validate`)
5. Execute (`ir_execute`)

## Builtins Supported
- `Write(...)` / `WriteLn(...)` (multiple args)
- `ReadLn(var)`
- `ReadFile(path)` → `String`
- `WriteFile(path, content)`

## IR Opcodes (additions)
- `IR_PRINT tX`
- `IR_PRINTLN tX` (or no arg → newline)
- `IR_READLN name`
- `IR_READ_FILE tDst = READ_FILE tPath`
- `IR_WRITE_FILE tPath, tContent`

## CLI
```
liminal run <file>
```

## Tests
- `exec_hello.lim` → prints `Hello, World!`
- `exec_add.lim` → reads two integers; prints sum

## Notes
- Interpreter supports ints, reals, strings; no function calls beyond builtins
- Truthiness: nonzero numbers, non-empty strings
- File I/O: simple blocking operations
