# Oracles

## Overview
- **Goal:** Provider-agnostic interface for text/LLM calls.
- **Providers:** `mock` (built-in), `ollama` (local HTTP API).
- **Recording/Replaying:** Controlled by mode; persists JSONL with hash + response.

## Environment Variables
- `LIMINAL_ORACLE_PROVIDER`: `mock` (default) or `ollama`
- `LIMINAL_OLLAMA_ENDPOINT`: default `http://localhost:11434`
- `LIMINAL_OLLAMA_MODEL`: default `gemma3:12b` (or `ministral-3:8b`)
- `LIMINAL_ORACLE_MODE`: `live` (default) | `record` | `replay`
- `LIMINAL_ORACLE_RECORDING`: path to JSONL recording file (default `./oracle_recordings.jsonl`)

## Config File (`liminal.ini`)
Simple `key=value` pairs supported:
```
provider=ollama
endpoint=http://localhost:11434
model=llama3
mode=record
recording=oracle_recordings.jsonl
```

## Mock Provider
- Queue responses: `oracle_mock_queue(o, "text", NULL);`
- Queue failures: `oracle_mock_queue(o, NULL, "error");`

## Recording & Replay
- Wrap provider: `oracle_with_recording(base, "record", path);`
- Prompts canonicalized (whitespace normalized) and hashed (SHA-256).
- JSONL format: `{"hash":"...","prompt":"...","response":"...","ok":true}`

## Ollama Provider (Text)
- Requires `curl` in PATH.
- POSTs to `<endpoint>/api/generate` with `{model,prompt,stream:false}`.
- Parses `response` field from JSON.

## Troubleshooting
- **curl not found:** install curl or set provider=mock.
- **replay prompt not found:** ensure canonicalized prompt matches; check whitespace.
- **Ollama refused:** verify endpoint/model and server running: `curl http://localhost:11434/api/tags`.

## Tests
- `liminal_oracle_tests` (with `TIMEOUT 5s`).
- `liminal_ask_tests` (mock-backed ask + fallback + UnwrapOr).
- Integration test gated by `LIMINAL_OLLAMA_TEST=1`.
