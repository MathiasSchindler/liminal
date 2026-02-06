# Intermediate Representation (IR)

## Overview
The IR is a simple, linear instruction form for deterministic constructs. It supports:
- Expressions (constants, variables, arithmetic, comparisons)
- Assignments
- Control flow (`if`, `while`)
- Functions (including the program body as a function)

## Data Structures
- `IrProgram` → vector of `IrFunc`
- `IrFunc` → name, instruction list, temp/label counters
- `IrInstr` → opcode, destination temp, args, string payload, real payload

### Opcodes
```
IR_NOP, IR_CONST_INT, IR_CONST_REAL, IR_CONST_STRING,
IR_LOAD_VAR, IR_STORE_VAR,
IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
IR_EQ, IR_NEQ, IR_LT, IR_GT, IR_LE, IR_GE,
IR_JUMP, IR_JUMP_IF_FALSE, IR_LABEL, IR_RET,
IR_PRINT, IR_PRINTLN, IR_READLN, IR_READ_FILE, IR_WRITE_FILE
```

## Text Format (printer)
```
func Basic
  t0 = CONST_INT 1
  t1 = CONST_INT 2
  t2 = CONST_INT 3
  t3 = MUL t1, t2
  t4 = ADD t0, t3
```

Labels print as `Lname:`; jumps print `JUMP Lname`, `JUMP_IF_FALSE tX, Lname`.

## Translation Rules (AST → IR)
- Literals → `CONST_INT/CONST_REAL/CONST_STRING`
- Identifiers → `LOAD_VAR name`
- Binary ops (`+ - * / div mod == != < > <= >=`) → corresponding binop
- Unary `-` → `0 - expr`; unary `not` → `expr == 0`
- Assignment `X := expr` → lower `expr`, then `STORE_VAR X`
- `if cond then A else B` → cond, `JUMP_IF_FALSE else`, lower A, `JUMP end`, `LABEL else`, lower B, `LABEL end`
- `while cond do body` → `LABEL loop`, cond, `JUMP_IF_FALSE end`, body, `JUMP loop`, `LABEL end`
- Program body lowered as a function named the program name; functions lowered similarly (params ignored for now)

## Validator
- Ensures jumps target defined labels within the function
- Detects duplicate labels

## API
- Builders: `ir_emit_*`
- Printer: `ir_program_print`
- Validator: `ir_validate`
- Translator: `ir_from_ast`

## Notes
- This IR is intentionally minimal and stable for snapshot tests.
- Future milestones may extend opcodes and translation to cover more language constructs.
