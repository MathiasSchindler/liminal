# Liminal Test Programs

Three progressive test suites for validating the Liminal compiler. Each program
is standalone and can be compiled/run independently.

## 01-atomic: One Feature at a Time

Each program isolates a single language construct. If something fails here,
the bug is in exactly one subsystem.  These are essentially valid Pascal and
can be cross-checked against `fpc` (Free Pascal Compiler) for the deterministic
subset.

| File | Tests | Expected output (key lines) |
|------|-------|-----------------------------|
| t01_hello | WriteLn, program structure | `Hello, World!` |
| t02_int_vars | Integer vars, assignment, `+` | `59` |
| t03_int_arith | `+` `-` `*` `div` `mod` unary `-` | `22 12 85 3 2 -17` |
| t04_real_arith | Real arithmetic, `/` | `5.14 1.14 6.28 1.57` |
| t05_booleans | `and` `or` `not` `=` `<>` | `False True False True False True` |
| t06_strings | String concat with `+` | `Hello, World!` |
| t07_comparisons | `=` `<>` `<` `>` `<=` `>=` | `True False True False True False` |
| t08_if_else | Simple if/then/else | `Positive` |
| t09_if_chain | Else-if chain | `Zero` |
| t10_for_loop | `for I := 1 to 5` | `1 2 3 4 5` |
| t11_while | `while` with begin/end block | `1 2 3 4 5` |
| t12_repeat | `repeat/until` | `1 2 3 4 5` |
| t13_function | Function with `Result :=` | `42` |
| t14_func_params | Multi-param functions, `;` separator | `7 30` |
| t15_recursion | Recursive factorial | `1 120 3628800` |
| t16_record | Record type, field access | `10 20` |
| t17_array | Dynamic array literal, indexing | `10 50` |
| t18_enum | Enum type, case on enum | `Go` |
| t19_case_int | Case on integer, else branch | `Wednesday` |
| t20_fstring | f-string interpolation | `Hello, Liminal! You are 1 years old.` |
| t21_local_vars | Function-local vars, for loop | `55` |
| t22_begin_end | Compound statement blocks | `X is greater than 5` then `10` |
| t23_writeln | Write vs WriteLn, multi-arg | multiple lines |
| t24_nested_calls | `Double(Triple(5))` | `30` |
| t25_optional | `?Integer`, `Nothing` | `42` then value for Nothing |
| t26_file_roundtrip_guard | ReadFile/WriteFile roundtrip guard | `OK` |
| t27_record_patch | Record mutation with boolean guard | `Maya active=True` |
| t28_schema_validation_path | `ask ... into` success+fallback path | two lines, second is `fallback` |

### Cross-checking with Free Pascal

The atomic tests (except t16 record literals, t17 array literals, t18 enum syntax,
t20 f-strings, t25 optionals) can be compiled with `fpc` after minor adjustments:

```bash
# Liminal
liminal run t01_hello.lim

# Free Pascal (for comparison)
fpc t01_hello.lim -o t01_hello && ./t01_hello
```

Differences to watch for:
- Liminal uses `types` block; Pascal uses `type`
- Liminal uses 0-indexed arrays; Pascal uses 1-indexed by default
- Liminal has `f'...'` strings; Pascal has no equivalent
- Liminal has `?T` optionals; Pascal has no equivalent
- Liminal uses `Result :=`; Pascal uses function-name assignment

## 02-combined: A Few Features Together

Each program combines 2-4 features. If atomics pass but a combined test fails,
the bug is in how subsystems interact.

| File | Combines | Expected output (key lines) |
|------|----------|-----------------------------|
| c01_fibonacci | functions, while, if/else, f-strings | `Fib(0) = 0` through `Fib(10) = 55` |
| c02_person_greet | records, functions, f-strings, if/else | `Good day, Alice.` / `Hey Bob!` |
| c03_traffic_light | enums, case, functions, for loop | 6 steps cycling R→G→Y→R→G→Y |
| c04_array_ops | arrays, for-in, functions | `Sum: 34 Max: 9 Min: 1` |
| c05_result_types | `!T`, `Ok()`, `Err()`, case | `10 / 3 = 3` / `Error: Division by zero` |
| c06_data_table | records, arrays, for-in, functions | Five students with grades, class average |
| c07_gcd_lcm | recursion, mod, multiple functions | `GCD(12,8) = 4` / coprime pairs |
| c08_constraints | constrained types, schema decl | `Rating: 3` / `Rating: 5` |
| c09_invoice_batch | records, arrays, aggregation function | `Batch total: 840` |
| c10_shift_alerting | enums, records, case routing in loop | `billing: log`, `api: page`, `search: notify` |
| c11_ticket_triage_extract | deterministic prompt build + typed extraction | structured output or `triage_failed` |

## 03-oracle: Into LLM Territory

These require the oracle subsystem (M9-M12). Use mock provider for deterministic
testing. Each program increases oracle complexity gradually.

| File | Tests | Oracle features used |
|------|-------|---------------------|
| o01_ask_simple | Simplest possible ask | `ask ... else` |
| o02_ask_result | Result handling | `TOracleResult<T>`, `case Ok/Err` |
| o03_ask_timeout | Timeout clause | `ask ... timeout ... else` |
| o04_ask_cost | Cost tracking | `with cost`, `TCost`, tuple destructuring |
| o05_ask_into | Structured extraction | `ask ... into`, `schema`, constrained fields |
| o06_ask_sequence | Multiple asks in a loop | `ask` + deterministic logic interleaved |
| o07_ask_into_enum | Schema with enum field | `schema` with enum, `ask ... into` |
| o08_fallback_chain | Cascading fallback | `ask ... else ask ... else literal` |

## 04-benchmark: Performance-Focused Deterministic Workloads

These are deterministic `t`/`c` style programs intended for rough throughput and
bottleneck checks while avoiding oracle/network variability.

| File | Focus | Expected output (key lines) |
|------|-------|-----------------------------|
| t29_bench_int_hotloop | tight integer loop + branch | `bench-int checksum=... final=...` |
| t30_bench_function_calls | hot function-call path | `bench-calls acc=...` |
| c12_bench_event_aggregation | records + for-in aggregation | `bench-events total=...` |
| c13_bench_rule_routing | enum case dispatch + loops | `bench-routing score=...` |

### Running oracle tests with mock provider

```bash
LIMINAL_ORACLE_PROVIDER=mock liminal run o01_ask_simple.lim
```

### Running oracle tests with Ollama

```bash
LIMINAL_ORACLE_PROVIDER=ollama liminal run o01_ask_simple.lim
```

## Debugging Strategy

1. Run all 01-atomic tests. Fix any failures before proceeding.
2. Run all 02-combined tests. Failures here = interaction bugs.
3. Run 03-oracle tests in order (o01 through o08). Each adds one oracle concept.

If a combined or oracle test fails but atomics pass, the bug is in how the
compiler/runtime composes features — typically scope handling, memory management
across function boundaries, or IR generation for compound expressions.
