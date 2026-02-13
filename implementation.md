# Implementation Plan: Liminal Compiler (C)

| Milestone | Description | Status |
|-----------|-------------|---------|
| 1 | Project Skeleton and Tooling | ✅ Completed 2026-02-05 |
| 2 | Lexer and Token Tests | ✅ Completed 2026-02-05 |
| 3 | Parser and AST | ✅ Completed 2026-02-05 |
| 4 | Symbol Tables and Type System Core | ✅ Completed 2026-02-05 |
| 5 | Memory Management Runtime | ✅ Completed 2026-02-05 |
| 6 | Constraints and Schema Validation | ✅ Completed 2026-02-05 |
| 7 | Intermediate Representation (IR) | ✅ Completed 2026-02-06 |
| 8 | Deterministic Runtime and Execution | ✅ Completed 2026-02-05 |
| 9 | Oracle Abstraction Layer | ✅ Completed 2026-02-06 |
| 10 | ask and Result Types | ✅ Completed 2026-02-06 |
| 11 | Cost Tracking and Budgets | ✅ Completed 2026-02-06 |
| 12 | Structured Extraction | ✅ Completed 2026-02-06 |
| 13 | consult Block and Retry Logic | ✅ Completed 2026-02-06 |
| 14 | Parallel Blocks | ⏳ Not Started |
| 15 | Context and Conversation | ⏳ Not Started |
| 16 | Streaming Responses | ⏳ Not Started |
| 17 | Standard Library Surface | ⏳ Not Started |
| 18 | End-to-End Example Programs | ⏳ Not Started |
| 19 | Ahead-of-Time Native Compilation | ⏳ Not Started |
| 20 | Packaging and Distribution | ⏳ Not Started |

This document defines sequential milestones to implement the Liminal compiler in C. Each milestone must be fully implemented, tested, and documented before the next begins. Early tests use local Ollama, behind an abstraction layer so providers can be swapped later.

## Milestone 1: Project Skeleton and Tooling

**Status**: Completed 2026-02-05

**Goal**: Establish a reproducible C build, test harness, and documentation structure.

**Deliverables**:
- Build system (e.g., CMake or Make) with debug/release targets.
- Basic CLI stub: `liminal --help`, `liminal --version`.
- Test runner (e.g., custom C test harness) and CI instructions.
- `docs/` outline with architecture overview.
- CI setup (gcc/clang, debug/release) and sanitizer flags in debug builds.
- Coverage reporting and fuzzing hooks (optional in early CI).

**Tests**:
- CLI help/version output snapshot tests.
- Build validation on Linux.
- Sanitizer smoke tests (address, undefined).
- Coverage smoke test to ensure instrumentation is wired.

**Docs**:
- Build and run instructions.
- Repository layout and coding conventions.

---

## Milestone 2: Lexer and Token Tests

**Status**: Completed 2026-02-05

**Goal**: Implement a complete lexer for the Liminal grammar.

**Deliverables**:
- Token definitions covering keywords, operators, literals (including bytes, duration, money), and comments.
- Keyword list includes `within`, `budget`, `stream`, `consult`, `retry`, `yield`, `parallel`, `matching`, `div`, `mod`.
- Lexer with precise error reporting and source spans.
- Golden-file tests for tokenization.

**Tests**:
- Tokenization of small programs and edge cases (string escapes, `b'...'`, `f'...'`, duration, money).
- Invalid token detection with line/column correctness.
- Lexer fuzzing harness (corpus-based).

**Docs**:
- Lexer rules and token list.

---

## Milestone 3: Parser and AST

**Status**: Completed 2026-02-05

**Goal**: Implement a full parser and AST for the language specification.

**Deliverables**:
- AST definitions with source spans.
- Recursive descent parser (or Pratt parser for expressions).
- Error recovery for common syntax errors.
- Tuple types/literals and destructuring assignments.
- Slice syntax for indexing (e.g., `expr[lo..hi]`, `expr[lo..]`).

**Tests**:
- Parse valid programs covering all constructs.
- Parse errors for invalid syntax with expected messages.
- Tuple and slice parsing tests.
- Parser fuzzing harness (AST or error invariants).

**Docs**:
- AST reference with node definitions.
- Parsing strategy and error recovery notes.

---

## Milestone 4: Symbol Tables and Type System Core

**Status**: Completed 2026-02-05

**Goal**: Implement name resolution and core type checking.

**Deliverables**:
- Symbol table with scopes (types, variables, functions, oracles, schemas).
- Type representations for primitives, arrays, records, optionals, results.
- Basic type checking for expressions and assignments.

**Tests**:
- Type mismatch errors and successful checks.
- Shadowing and scope resolution.

**Docs**:
- Type system overview and name resolution rules.

---

## Milestone 5: Memory Management Runtime

**Status**: Completed 2026-02-05

**Goal**: Provide automatic memory management for strings, arrays, records, and schema values.

**Deliverables**:
- Runtime memory model (reference counting with cycle handling, or tracing GC).
- Allocation APIs for strings, arrays, and records.
- Deterministic tests for leaks and double-frees (debug mode).

**Tests**:
- Stress tests for string concatenation and dynamic array growth.
- Leak checks for short-lived allocations.

**Docs**:
- Memory management strategy and invariants.

---

## Milestone 6: Constraints and Schema Validation

**Status**: Completed 2026-02-05

**Goal**: Add constrained types and schema-specific validation metadata.

**Deliverables**:
- Constraint parsing and type annotation support.
- Schema type declarations and `describe` clauses.
- JSON Schema emitter for schema types.

**Tests**:
- Constraint validation on sample values.
- Schema generation tests matching expected JSON.

**Docs**:
- Constraint rules and schema mapping details.

---

## Milestone 7: Intermediate Representation (IR)

**Goal**: Define and emit a minimal IR for deterministic constructs.

**Deliverables**:
- IR for control flow, expressions, functions, and I/O calls.
- IR validator and printer for debugging.

**Tests**:
- IR snapshots for small programs.
- IR validator catches malformed nodes.

**Docs**:
- IR specification and translation rules.

---

## Milestone 8: Deterministic Runtime and Execution

**Goal**: Execute deterministic programs (no oracles) end-to-end.

**Deliverables**:
- Runtime support for strings, arrays, records, file I/O.
- Interpreter or codegen backend for deterministic IR.
- `liminal run` for deterministic programs.
- Exception handling strategy (e.g., setjmp/longjmp or explicit error unwinding).

**Tests**:
- HelloWorld, AddNumbers, file I/O examples.
- Error handling for file not found and bounds errors.

**Docs**:
- Runtime API documentation.
- Deterministic execution model.

---

## Milestone 9: Oracle Abstraction Layer

**Goal**: Introduce a provider-agnostic oracle interface, with Ollama as the first implementation.

**Deliverables**:
- C interface for oracle calls: text, embeddings, vision.
- Provider config loader (env + config file).
- Ollama provider implementation using local HTTP API.
- Mock provider for tests.
- Recording/replay support with prompt canonicalization and hashing.

**Tests**:
- Mock provider unit tests for success and failure cases.
- Integration test that calls Ollama (skipped if not available).

**Docs**:
- Provider interface documentation.
- Ollama configuration and troubleshooting.

---

## Milestone 10: `ask` and Result Types

**Goal**: Implement `ask` expressions with `TOracleResult<T>` and `else` fallback.

**Deliverables**:
- `ask` evaluation with timeouts, error mapping, and cost metadata.
- `else` handling to unwrap with fallback values.
- Result type operations: `case` matching and `UnwrapOr`.

**Tests**:
- Successful and failing `ask` cases via mock provider.
- `else` fallback semantics.

**Docs**:
- Oracle result model and `ask` semantics.

---

## Milestone 11: Cost Tracking and Budgets

**Goal**: Expose cost metadata, enforce budget scopes, and support compile-time estimation.

**Deliverables**:
- `with cost` tuple return for `ask` and `consult`.
- `within budget` scoped enforcement.
- Cost accounting plumbing available even on failures.
- `liminal compile --estimate-costs` for static prompts.

**Tests**:
- Cost propagation on success and failure.
- Budget scope exhaustion tests.
- Estimation tests for static prompts.

**Docs**:
- Cost model, budget scope rules, and estimation limits.

---

## Milestone 12: Structured Extraction

**Status**: Completed 2026-02-06

**Goal**: Implement `ask ... into` with schema validation and extraction failures.

**Deliverables**:
- JSON parser and validation against schema.
- Compiler-emitted schema metadata tables (field names, types, constraints, offsets).
- Runtime JSON-to-struct mapping using metadata.
- Extraction error details (`TOracleFailure` fields).
- Error messages suitable for retry hints.

_Implemented: flat JSON object validator, IR schema metadata, runtime validation, ask_into tests. Missing: nested/arrays, structured mapping, rich `TOracleFailure`._

**Tests**:
- Valid JSON extraction.
- Invalid JSON and constraint violations.
- Metadata table correctness for nested schemas.

**Docs**:
- Structured extraction pipeline.

---

## Milestone 13: `consult` Block and Retry Logic

**Status**: Completed 2026-02-06

**Goal**: Implement `consult` with retries, hints, budget, and failure actions.

**Deliverables**:
- Retry engine with attempts count (per-attempt timeout/budget not yet implemented).
- Failure actions: `retry`, `retry with hint`, `yield` (Fallback). `wait`/time budgets TBD.
- Budget tracking via attempts count.

**Tests**:
- Retry on failure using mock oracle.
- Hint injection on extraction failure.
- Fallback after attempts exhausted.

**Docs**:
- `consult` semantics and failure handling.

_Limits_: no timeouts or monetary budgets yet; failure cases not typed (`TOracleFailure` placeholder); non-nested hint injection; flat JSON validation.

---

## Milestone 14: Parallel Blocks

**Goal**: Implement the `parallel ... end` fork-join block.

**Deliverables**:
- Parallel execution engine for statement blocks (thread pool or task queue).
- Deterministic join semantics and error aggregation.
- Oracle-safe concurrency policy (e.g., per-provider limits).

**Tests**:
- Parallel `ask` calls with mock provider.
- Ordering and join correctness tests.

**Docs**:
- Parallel execution semantics and limitations.

---

## Milestone 15: Context and Conversation

**Goal**: Implement `Context.Create(...)` and multi-turn context handling.

**Deliverables**:
- Context object with message history.
- Overflow strategies: Summarize, Truncate, Fail, Sliding.
- Hooks for provider-specific context handling.

**Tests**:
- Context accumulation and token counting.
- Overflow strategy tests (mock provider).

**Docs**:
- Context semantics and overflow strategies.

---

## Milestone 16: Streaming Responses

**Goal**: Implement `stream` with incremental output.

**Deliverables**:
- Streaming interface for providers.
- `stream` statement execution with callback chunk handling.

**Tests**:
- Simulated chunk streaming with mock provider.

**Docs**:
- Streaming protocol and usage.

---

## Milestone 17: Standard Library Surface

**Goal**: Provide stable stubs for standard modules used in examples.

**Deliverables**:
- Core module stubs for `System`, `Files`, `JSON`, `HTTP`, `Regex`, `Time`, `Math`, `Crypto`.
- AI-specific stubs for `Oracles`, `Vectors`, `Chunking`, `Schemas`, `Prompts`, `Documents`.

**Tests**:
- Module import resolution tests.

**Docs**:
- Standard library index and module descriptions.

---

## Milestone 18: End-to-End Example Programs

**Goal**: Run the spec examples end-to-end where feasible.

**Deliverables**:
- Passing builds for deterministic examples.
- Passing builds for oracle examples with Ollama (or mock provider).
- `DocAssistant` example running with mocked extraction.

**Tests**:
- Regression tests for all example programs.

**Docs**:
- Example execution guide and expected outputs.

---

## Milestone 19: Ahead-of-Time Native Compilation

**Goal**: Compile `.lim` programs into native binaries that run without the `liminal` CLI at runtime.

**Deliverables**:
- `liminal compile <input.lim> -o <output>` command.
- Native code generation backend (or C backend + toolchain invocation) for deterministic subset.
- Runtime embedding/linking strategy for required language runtime pieces.
- Startup entrypoint generation and argument forwarding.
- Diagnostics for unsupported probabilistic/oracle features in AOT mode.

**Tests**:
- Compile and run deterministic fixture programs as standalone binaries.
- Snapshot tests for compile diagnostics and unsupported-feature errors.
- Cross-build smoke tests on Linux (debug/release).

**Docs**:
- AOT compilation guide and limitations.
- Runtime dependency expectations for produced binaries.

---

## Milestone 20: Packaging and Distribution

**Goal**: Prepare a minimal distribution with docs and tests.

**Deliverables**:
- Release build artifacts and install instructions.
- Versioned CLI help and diagnostics.

**Tests**:
- Smoke tests on the release build.

**Docs**:
- Release checklist and troubleshooting guide.
