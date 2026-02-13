# Liminal Compiler (C)

> **Note**: This project was written with the help of an LLM and is released under **CC0** as an experiment.

## What is Liminal?
Liminal is a small, friendly language that keeps **deterministic code** and **probabilistic/LLM code** visibly separate. It feels familiar if you’ve seen Pascal, but adds:
- Result/optional types (`!T`, `?T`)
- Records, arrays, enums, tuples, f-strings
- Constrained types (`Integer[1..5]`, `String[1..100]`)
- Explicit `ask`/`consult` for LLM calls with schemas, retries, and budgets

Think of it as “honest syntax” for programs that sometimes talk to models.

## Why another language?
Traditional languages blur deterministic code and LLM calls. Liminal makes the boundary explicit:
- Deterministic by default; LLM features are opt-in
- Costs and failures are first-class (`ask ... else`, `consult`, budgets)
- Types drive extraction and validation (`ask ... into Schema`)
- Testing support: mock/record/replay oracles

## Project status
We’re progressing milestone-by-milestone. For details, see **[implementation.md](implementation.md)**. For the language design, see **[liminal-spec.md](liminal-spec.md)**.

Highlights implemented:
- Lexer, parser, IR, deterministic runtime
- Result/optional types, records/arrays/enums/tuples
- Constrained types & schemas
- `ask`, `ask ... into`, `consult` (mock/oracles abstraction)
- Test suites and example programs

Planned next major capability:
- Ahead-of-time native compilation (`liminal compile ...`) is tracked as Milestone 19 in `implementation.md`.

## Quickstart
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cd build
ctest --output-on-failure
```

Run examples:
```bash
# Atomics t01–t25
for f in ../examples/opus/t{01..25}_*.lim; do ./src/liminal run "$f"; done

# Combined c01–c08
for f in ../examples/opus/c0{1..8}_*.lim; do ./src/liminal run "$f"; done
```

## Layout
- `src/` — compiler front-end, IR, runtime
- `include/` — public headers
- `tests/` — CLI, lexer, parser, typecheck, runtime, schema tests
- `examples/opus/` — atomic and combined feature programs
- `docs/` — build/testing/architecture
- `.github/workflows/` — CI

## Features at a glance
- **Deterministic core:** Pascal-like syntax, 0-index arrays, records, enums, tuples
- **Types:** `!T` results, `?T` optionals, constrained types, schemas
- **Strings:** f-strings (`f'Hello {Name}'`), concat, slices
- **Oracles:** `ask`, `ask ... into`, `consult` with retries/hints/budgets
- **Costs:** explicit budgets, cost reporting (mock providers for tests)

## Learn more
- Language spec: **[liminal-spec.md](liminal-spec.md)**
- Implementation plan & milestones: **[implementation.md](implementation.md)**
- Build/test docs: `docs/BUILD.md`, `docs/TESTING.md`

Have fun exploring! Friendly PRs and experiments welcome.
