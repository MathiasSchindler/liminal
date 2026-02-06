# Architecture Overview (Outline)

- **Goals**: Honest syntax for deterministic vs probabilistic computation; deterministic core first.
- **Milestone Progression**: See `implementation.md` for sequential milestones.
- **Compiler Layers**:
  - Frontend: Lexer, Parser, AST (implemented)
  - Semantic: Symbol tables, Types, Constraints (symbols/types implemented)
  - IR: Deterministic IR for execution (see `docs/IR.md`)
  - Runtime: Deterministic runtime, Memory management (refcounted strings/arrays)
  - Execution: Interpreter for deterministic subset (see `docs/EXECUTION.md`)
  - Oracle Abstraction: Provider interface, mock/record/replay
- **Runtime**:
  - Memory model TBD (RC or tracing GC)
  - Error handling: Result types vs Exceptions
  - Structured extraction: schemas lowered into IR metadata and validated at runtime (flat JSON objects)
  - Consult: retry loop lowered to IR with attempts, hints, fallback
- **Tooling**:
  - CMake build, custom test harness, CI workflows

_Fill in details as milestones progress._
