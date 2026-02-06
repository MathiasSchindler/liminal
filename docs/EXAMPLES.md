# Examples

Run with `liminal run examples/<file>`

| #  | File                    | Highlights                              | Notes |
|----|-------------------------|-----------------------------------------|-------|
| 01 | `01_hello.lim`          | Hello World                             | Deterministic |
| 02 | `02_add.lim`            | Input/Output                            | Pipe input for tests |
| 03 | `03_file_io.lim`        | Read/Write files                        | Writes `out.txt` |
| 04 | `04_ask_basic.lim`      | `ask ... else` fallback                 | Mock oracle or fallback |
| 05 | `05_ask_with_cost.lim`  | `with cost` tuple                       | Mock/Ollama cost=0 |
| 06 | `06_budget.lim`         | `within budget`                         | Uses fallback |

### Testing
```bash
# All tests
cd build && ctest --output-on-failure --timeout 10

# Examples smoke
liminal run examples/01_hello.lim
printf '3\n4\n' | liminal run examples/02_add.lim
liminal run examples/04_ask_basic.lim   # fallback prints Ok(fallback)
```
