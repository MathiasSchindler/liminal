# Runtime Memory Management

## Model
- Reference counting with manual cycle avoidance (no cycle detection yet)
- Refcounted objects: `LString`, `LArray`
- Arrays hold `LValue` elements (strings or arrays)
- Primitive counters for allocations/frees (tests)

## API (`runtime.h`)
- `lstring_new`, `lstring_from_cstr`
- `larray_new`, `larray_push`, `larray_get`, `larray_set`
- `lobject_retain`, `lobject_release`
- `runtime_reset_counters`, `runtime_alloc_count`, `runtime_free_count`

## Tests
- `tests/test_runtime.c`
  - String refcount lifecycle
  - Array growth and element retention
  - Array set replaces with proper release

## Notes / Future
- Add cycle detection or tracing GC for records/contexts
- Add schemas/records runtime representations
- Optionally add arena bump allocator for short-lived objects
