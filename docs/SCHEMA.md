# Constraints & Schema

## Constraints
- Forms:
  - `Integer[1..5]` => `minimum`, `maximum`
  - `String[1..100]` => `minLength`, `maxLength`
  - `String matching 'regex'`
  - `array[1..5] of T` => `minItems`, `maxItems` (stored in ASTArrayType)

## Schema Declaration
```
types
  schema Person
    Name: String[1..100] describe 'full name';
    Age: Integer[0..120];
  end;
```

## JSON Schema Emission
- `schema_to_json(ASTType* schema_type)` outputs JSON Schema string
- Properties include constraints and `description`

Example output:
```
{"type":"object","properties":{
  "Name":{"type":"String","minimum":1,"maximum":100,"minLength":1,"maxLength":100,"description":"full name"},
  "Age":{"type":"Integer","minimum":0,"maximum":120}
},"required":["Name","Age"]}
```

## Tests
- `tests/test_schema_emit.c` compares emitted JSON to fixtures in `tests/fixtures/schema_basic.{lim,json}`

## Notes
- Semantic validator not implemented yet; schema emitter is milestone scope.
- Type names are emitted as-is (String/Integer) for now; mapping to JSON Schema types may be refined.
