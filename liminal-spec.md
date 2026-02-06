# The Liminal Programming Language

**Version:** 0.1 (Draft)  
**Status:** Request for Comments  
**Paradigm:** Imperative, Structured, with Explicit Probabilistic Extensions

---

## 1. Design Philosophy

Liminal is a programming language for the era when computation includes both deterministic algorithms and probabilistic inference. Its core principle is **honesty**: the syntax never hides what's actually happening.

### 1.1 The Two Worlds

Liminal recognizes that programs now operate in two distinct computational regimes:

| Deterministic World | Probabilistic World |
|---------------------|---------------------|
| Reproducible | Non-reproducible by nature |
| Fast (nanoseconds) | Slow (milliseconds to seconds) |
| Free after compilation | Costs money per call |
| Testable with assertions | Testable only with distributions |
| Fails loudly | Fails silently or weirdly |

Most languages pretend these are the same. Liminal makes the boundary explicit and uncrossable without ceremony.

### 1.2 Core Principles

1. **Honest Syntax**: Operations that are expensive, slow, or non-deterministic must *look* different from those that aren't. You cannot hide an API call inside an operator.

2. **Deterministic by Default**: A Liminal program with no `oracle` blocks is a normal, fully deterministic program. LLM features are opt-in, not ambient.

3. **Type-Driven Extraction**: When you ask an LLM for structured data, the type system defines the contract. The compiler generates validation, retry logic, and error messages.

4. **Cost Visibility**: Every probabilistic operation has an explicit cost site. The compiler can generate cost estimates. The runtime can enforce budgets.

5. **Testability First**: Every oracle can be mocked, recorded, and replayed. Testing probabilistic code is a first-class concern.

6. **Graceful Degradation**: The language has built-in patterns for "what if the LLM fails?" because it will.

---

## 2. Program Structure

A Liminal program consists of these top-level blocks, all optional except `begin...end`:

```pascal
program ProgramName;

uses      // Module imports
config    // Compile-time configuration
types     // Type definitions (including schemas)
oracles   // LLM endpoint declarations
var       // Global variables
          // Function definitions appear here (no 'functions' keyword)

begin
  // Main program
end.
```


**Semantics:** Exceptions unwind the stack to the nearest matching handler. They are not used for oracle failures.
Function definitions appear at the top level without a grouping keyword:

```pascal
program Example;

var
  X: Integer;

function Double(N: Integer): Integer;
begin
  Result := N * 2;
end;

function Triple(N: Integer): Integer;
begin
  Result := N * 3;
end;

begin
  X := Double(Triple(5));
  WriteLn(X);  // 30
end.
```

### 2.1 Minimal Programs

Liminal works as a normal programming language:

```pascal
program HelloWorld;
begin
  WriteLn('Hello, World!');
end.
```

```pascal
program AddNumbers;
var
  A, B: Integer;
begin
  Write('First number: ');
  ReadLn(A);
  Write('Second number: ');
  ReadLn(B);
  WriteLn('Sum: ', A + B);
end.
```

No LLMs required. No configuration. Just a program.

---

## 3. The Type System

### 3.1 Primitive Types

| Type | Description | Example |
|------|-------------|---------|
| `Integer` | 64-bit signed integer | `42` |
| `Real` | 64-bit float | `3.14159` |
| `Boolean` | True or False | `True` |
| `Char` | Single UTF-8 codepoint | `'λ'` |
| `String` | UTF-8 text, immutable | `'Hello'` |
| `Byte` | 8-bit unsigned integer | `255` |
| `Bytes` | Raw byte sequence | `b'\x00\xFF'` |

### 3.2 Compound Types

```pascal
types
  // Records (product types)
  TPerson = record
    Name: String;
    Age: Integer;
  end;

  // Dynamic arrays (0-indexed)
  TNumbers = array of Integer;
  
  // Fixed arrays (0-indexed, size specified)
  TBuffer = array[100] of Byte;      // Indices 0..99
  TMatrix = array[10, 10] of Real;   // 10x10, indices 0..9 in each dimension

  // Tuples
  TPair = tuple<Integer, String>;

  // Enumerations
  TColor = (Red, Green, Blue);

  // Optionals (nullable types)
  TMaybeInt = ?Integer;  // Either an Integer or Nothing

  // Results (success or typed error)
  TParseResult = !Integer;  // Either Integer or Error
```

**Indexing convention:** All arrays and strings in Liminal are 0-indexed. The notation `array[N]` declares an array of N elements with indices 0 through N-1.

### 3.3 Constrained Types (For LLM Extraction)

Liminal allows constraints on types. These constraints are:
- Checked at compile time where possible
- Enforced at runtime always
- Communicated to LLMs via schema injection

```pascal
types
  // String with length constraint (character count)
  TSummary = String[1..500];  // 1 to 500 characters
  
  // Integer with range constraint  
  TRating = Integer[1..5];    // Value must be 1, 2, 3, 4, or 5
  
  // String with pattern constraint
  TEmail = String matching '[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}';
  
  // Array with length constraint (element count)
  TTopics = array[1..5] of String[1..50];  // 1 to 5 topics, each ≤50 chars
```

**Notation clarification:** 
- `Integer[1..5]` means the *value* must be in range 1 to 5 (a constraint)
- `array[100]` means an array of *size* 100 (a fixed-size declaration)  
- `array[1..5] of T` means an array with *length* between 1 and 5 (a constraint)

### 3.4 Schema Records

A `schema` is a record specifically designed for LLM extraction. The compiler generates:
- JSON Schema for prompt injection
- Validation functions
- Constraint-specific error messages for retry hints

```pascal
types
  schema TCustomerIntent
    Summary: String[1..200]
      describe 'A one-sentence summary of the customer request';
    
    Urgency: Integer[1..10]
      describe 'How urgent is this? 1=whenever, 10=emergency';
    
    Category: (Billing, Technical, Sales, Other)
      describe 'The primary category of the request';
    
    RequiresHuman: Boolean
      describe 'True if this needs human escalation';
  end;
```

The `describe` clauses become part of the injected schema, guiding the LLM.

**Note:** Schema fields use semicolons like record fields. The schema ends with `end;`.

### 3.5 Memory Management

Liminal uses automatic memory management. User code does not call `Free`, `Release`, or pointer operations. Implementations must ensure that strings, arrays, records, and schemas are reclaimed when they are no longer reachable.

The language does not expose finalizers or deterministic destruction. Implementations may use reference counting (with cycle handling) or a tracing garbage collector, but the observable program behavior is the same.

---

## 4. Deterministic Operations

Everything in this section is normal, reproducible computation.

### 4.1 Variables and Assignment

```pascal
var
  X: Integer;
  Name: String;
  Items: array of String;
begin
  X := 42;
  Name := 'Liminal';
  Items := ['one', 'two', 'three'];
end.
```

### 4.2 Control Flow

```pascal
// Conditionals
if X > 0 then
  WriteLn('Positive')
else if X < 0 then
  WriteLn('Negative')  
else
  WriteLn('Zero');

// Case/Match
case Color of
  Red: WriteLn('Stop');
  Green: WriteLn('Go');
  Blue: WriteLn('Sky');
end;

// Loops
for I := 1 to 10 do
  WriteLn(I);

for Item in Items do
  WriteLn(Item);

while Condition do
  DoSomething;

repeat
  DoSomething
until Condition;
```

### 4.3 Functions

Functions in Liminal are deterministic. Same inputs, same outputs, always.

```pascal
function Factorial(N: Integer): Integer;
begin
  if N <= 1 then
    Result := 1
  else
    Result := N * Factorial(N - 1);
end;

function ParseCSVLine(Line: String): array of String;
var
  Fields: TStringList;
begin
  Fields := Line.Split(',');
  Result := Fields.ToArray;
end;
```

### 4.4 String Operations

Liminal has rich deterministic string handling:

```pascal
var
  S: String;
  Parts: array of String;
begin
  S := '  Hello, World!  ';
  
  // Basic operations
  S := S.Trim;                    // 'Hello, World!'
  S := S.Upper;                   // 'HELLO, WORLD!'
  S := S.Lower;                   // 'hello, world!'
  S := S.Replace('world', 'Liminal');
  
  // Searching
  if S.Contains('hello') then ...
  if S.StartsWith('Hello') then ...
  if S.Matches('[A-Z][a-z]+') then ...  // Regex
  
  // Splitting and joining
  Parts := S.Split(', ');
  S := Parts.Join(' | ');
  
  // Slicing (0-indexed, half-open intervals)
  S := 'Hello'[0..2];  // 'Hel'
  S := 'Hello'[2..];   // 'llo'
  
  // String interpolation
  S := f'The answer is {X} and the name is {Name}';
end;
```

### 4.5 File and Stream I/O

```pascal
var
  Content: String;
  Lines: array of String;
  Data: Bytes;
begin
  // Read entire file
  Content := ReadFile('input.txt');
  
  // Read lines lazily
  for Line in ReadLines('input.txt') do
    ProcessLine(Line);
  
  // Write file
  WriteFile('output.txt', Content);
  
  // Binary I/O
  Data := ReadBytes('image.png');
  WriteBytes('copy.png', Data);
  
  // Structured data (JSON, CSV, etc.)
  Config := ReadJSON('config.json', TConfigRecord);
  WriteJSON('output.json', Result);
end;
```

---

## 5. The Probabilistic World

Here is where Liminal diverges from traditional languages. Everything in this section involves non-deterministic computation.

### 5.1 Oracles

An **oracle** is a connection to an external intelligence—an LLM endpoint. Oracles are declared explicitly:

```pascal
oracles
  // Local model via Ollama
  Fast: TextOracle = 'llama3:8b' via Ollama('http://localhost:11434');
  
  // Remote model via OpenAI
  Smart: TextOracle = 'gpt-4o' via OpenAI(env.OPENAI_API_KEY);
  
  // Embedding model
  Embedder: EmbedOracle = 'text-embedding-3-small' via OpenAI(env.OPENAI_API_KEY);
  
  // Vision model  
  Vision: VisionOracle = 'gpt-4o' via OpenAI(env.OPENAI_API_KEY);
```

Oracle types:
- `TextOracle`: Text in, text out
- `EmbedOracle`: Text in, vector out
- `VisionOracle`: Text + images in, text out
- `StructuredOracle`: Text in, typed data out (uses a TextOracle internally)

### 5.2 The `ask` Expression

The fundamental probabilistic operation is `ask`. It is syntactically distinct from function calls to make the cost visible.

`ask` always returns a `TOracleResult<T>`. Use `case` to handle it, or use `else` to unwrap with a fallback value.

```pascal
var
  Result: TOracleResult<String>;
begin
  // ask is NOT a function call. It's a different thing.
  Result := ask Smart <- 'What is the capital of France?';
  case Result of
    Ok(Response): WriteLn(Response);
    Err(F): WriteLn(f'Failed: {F.Message}');
  end;
end.
```

The `<-` operator emphasizes: this is a *request* to an external system, not a computation.

### 5.3 Structured Extraction with `ask...into`

When you need typed data, use `ask...into`. This returns a `TOracleResult<T>`.
The compiler emits schema metadata (field names, types, constraints, offsets) so the runtime can map JSON to the correct layout.

```pascal
var
  IntentResult: TOracleResult<TCustomerIntent>;
  Email: String;
begin
  Email := ReadFile('email.txt');
  
  // The compiler:
  // 1. Generates JSON schema from TCustomerIntent
  // 2. Injects it into the prompt
  // 3. Parses and validates the response
  // 4. Returns Err(ExtractionFailed) if it can't extract valid data
  
  IntentResult := ask Smart <- f'Analyze this email: {Email}' into TCustomerIntent;
  
  case IntentResult of
    Ok(Intent):
      WriteLn(f'Urgency: {Intent.Urgency}');
      WriteLn(f'Category: {Intent.Category}');
    Err(F):
      WriteLn(f'Extraction failed: {F.Message}');
  end;
end.
```

### 5.4 The `consult` Block (Retry with Hints)

LLMs fail. The `consult` block is structured retry logic for extraction. It is an expression that returns `TOracleResult<T>`.

```pascal
var
  Result: TOracleResult<TCustomerIntent>;
begin
  Result := consult Smart from
    f'Analyze this customer email:\n{EmailBody}'
  into TCustomerIntent
  with
    attempts: 3,
    timeout: 30s,
    budget: $0.10  // Hard cost limit
  on failure(F: TOracleFailure)
    case F.Kind of
      ExtractionFailed:
        // Return a hint string to inject into the retry prompt
        retry with hint f'The field {F.Details.Field} must satisfy: {F.Details.Constraint}. You provided: {F.Details.Value}';
      NetworkError:
        wait 1s;
        retry;
      Timeout:
        // Give up and use fallback
        yield DefaultIntent;
      BudgetExceeded:
        // Give up with error
        yield Err(F);
    end;
  end;
  
  // Result is TOracleResult<TCustomerIntent>
  case Result of
    Ok(Intent): ProcessIntent(Intent);
    Err(F): WriteLn(f'Failed after retries: {F.Message}');
  end;
end;
```

The `consult` block makes explicit:
- How many attempts maximum
- What timeout per attempt  
- What budget limit for all attempts combined
- What to do on each failure kind:
  - `retry` — try again (optionally `with hint` to inject guidance)
  - `retry with hint '...'` — try again with additional context
  - `wait Ns` — pause before continuing
  - `yield Value` — stop retrying, return this value as Ok
  - `yield Err(F)` — stop retrying, return failure

### 5.5 Embeddings and Similarity

Embeddings are **not operators**. They are explicit computations with visible cost:

```pascal
var
  V1, V2: Vector;      // Vector is a built-in type
  Similarity: Real;
begin
  // Each 'embed' is a visible operation
  V1 := embed Embedder <- 'I want to cancel my subscription';
  V2 := embed Embedder <- 'Please terminate my account';
  
  // Cosine similarity is a normal, deterministic function
  Similarity := V1.CosineSimilarity(V2);
  
  if Similarity > 0.85 then
    WriteLn('These are semantically similar');
end;
```

For comparing against a known set:

```pascal
var
  Intents: TVectorIndex;
  Match: TIndexMatch;
begin
  // Build index (expensive, do once)
  Intents := VectorIndex.Create(Embedder);
  Intents.Add('cancel', 'I want to cancel');
  Intents.Add('refund', 'I want my money back');
  Intents.Add('help', 'I need assistance');
  
  // Query (one embedding call)
  Match := Intents.Search(UserInput, threshold: 0.80);
  
  if Match.Found then
    WriteLn(f'Matched intent: {Match.Key} with score {Match.Score}')
  else
    WriteLn('No matching intent found');
end;
```

### 5.6 Vision

```pascal
var
  Image: Bytes;
  Description: String;
begin
  Image := ReadBytes('photo.jpg');
  
  Description := ask Vision <- (
    text: 'Describe what you see in this image.',
    images: [Image]
  );
  
  WriteLn(Description);
end.
```

### 5.7 Streaming Responses

For long responses, use `stream`:

```pascal
begin
  stream Smart <- 'Write a short story about a robot.' do |Chunk|
    Write(Chunk);  // Print incrementally
  end;
  WriteLn;
end.

### 5.8 Parallel Blocks

LLM calls are high-latency. Use `parallel` for a simple fork-join block where statements may run concurrently. Execution joins at the end of the block, and results are available afterward.

Error aggregation: `parallel` does not short-circuit. If multiple operations fail, each result remains available. Do not assume a specific interleaving of side effects.

```pascal
var
  R1, R2: TOracleResult<String>;
begin
  parallel
    R1 := ask Smart <- 'Summarize document A.';
    R2 := ask Smart <- 'Summarize document B.';
  end;

  case R1 of
    Ok(S1): WriteLn(S1);
    Err(F1): WriteLn(f'Failed A: {F1.Message}');
  end;
  case R2 of
    Ok(S2): WriteLn(S2);
    Err(F2): WriteLn(f'Failed B: {F2.Message}');
  end;
end.
```
```

---

## 6. Context and Conversation

### 6.1 The `context` Block

When you need multi-turn conversation or persistent context:

```pascal
var
  Ctx: TContext;
  Result: TOracleResult<String>;
begin
  Ctx := Context.Create(Smart)
    .WithSystem('You are a helpful coding assistant.')
    .WithMaxTokens(4096);
  
  // Each ask within the context remembers history
  Result := ask Ctx <- 'How do I read a file in Python?';
  case Result of
    Ok(Response): WriteLn(Response);
    Err(F): WriteLn(f'Failed: {F.Message}');
  end;
  
  Result := ask Ctx <- 'What about writing to it?';  // Remembers previous exchange
  case Result of
    Ok(Response): WriteLn(Response);
    Err(F): WriteLn(f'Failed: {F.Message}');
  end;
  
  // Inspect context state
  WriteLn(f'Tokens used: {Ctx.TokenCount}');
  WriteLn(f'Estimated cost: ${Ctx.EstimatedCost}');
end.
```

### 6.2 Context Window Management

```pascal
var
  Ctx: TContext;
  Result: TOracleResult<String>;
begin
  Ctx := Context.Create(Smart)
    .WithMaxTokens(8192)
    .OnOverflow(strategy: Summarize);  // or: Truncate, Fail, Sliding
  
  // When context exceeds limit, strategy kicks in automatically
  for Message in LoadChatHistory do
    Result := ask Ctx <- Message.Text;
    // Handle result as needed
  end;
end.
```

---

## 7. Cost Tracking and Budgets

### 7.1 Cost Expressions

Every `ask`, `embed`, and `consult` can return cost information alongside the result. `with cost` returns a tuple `(TOracleResult<T>, TCost)` and can be destructured:

```pascal
var
  Result: TOracleResult<String>;
  Cost: TCost;
begin
  // Use 'with cost' to get cost information
  (Result, Cost) := ask Smart <- 'Hello' with cost;
  
  case Result of
    Ok(Response):
      WriteLn(f'Response: {Response}');
      WriteLn(f'Input tokens: {Cost.InputTokens}');
      WriteLn(f'Output tokens: {Cost.OutputTokens}');
      WriteLn(f'Estimated price: ${Cost.EstimatedPrice}');
    Err(F):
      WriteLn(f'Failed: {F.Message}');
      // Cost still available for failed requests (tokens used before failure)
      WriteLn(f'Tokens used before failure: {Cost.InputTokens}');
  end;
end.
```

### 7.2 Budget Scopes

```pascal
begin
  // Set a budget for a block of code
  within budget $1.00 do
    // Multiple operations...
    R1 := ask Smart <- Query1;
    R2 := ask Smart <- Query2;
    R3 := ask Smart <- Query3;
  on exceeded do
    WriteLn('Budget exhausted, stopping.');
  end;
end.

**Nesting rules:** Budget scopes are additive. Inner scopes draw from their own budget and also count toward any enclosing budget. On exceeded, only the innermost scope's handler runs.
```

### 7.3 Compile-Time Cost Estimation

The compiler can estimate costs for static prompts:

```
$ liminal --estimate-cost myprogram.lim

Cost Estimate for myprogram.lim:
  Line 42: ask Smart <- '...' 
           ~500 input tokens, ~200 output tokens
           Estimated: $0.0035 per call
  
  Line 67: consult Smart (3 attempts max)
           Worst case: $0.0105
  
  Total worst-case cost per execution: $0.0892
```

---

## 8. Testing and Reproducibility

### 8.1 Recording and Replay

```pascal
program TestableProgram;

config
  // In test mode, use recorded responses
  OracleMode = env.ORACLE_MODE ?? 'live';  // 'live', 'record', 'replay'
  RecordingPath = './fixtures/oracle_responses.json';

oracles
  Smart: TextOracle = 'gpt-4o' via OpenAI(env.OPENAI_API_KEY);

begin
  // When OracleMode = 'record':
  //   - Makes real API calls
  //   - Saves (prompt, response) pairs to RecordingPath
  
  // When OracleMode = 'replay':
  //   - Reads from RecordingPath
  //   - Fails if prompt not found in recording
  //   - Uses provider-specific prompt canonicalization and hashing
  
  // When OracleMode = 'live':
  //   - Normal operation
end.
```

### 8.2 Mock Oracles

```pascal
program UnitTests;

uses
  Testing;

oracles
  // MockOracle returns predefined responses
  Smart: TextOracle = MockOracle([
    ('What is 2+2?', '4'),
    ('What is the capital of France?', 'Paris'),
    (any, 'I don\'t know')  // Default response
  ]);

test 'Extraction works correctly';
var
  Result: TOracleResult<TCustomerIntent>;
begin
  Smart.QueueResponse('{"Summary": "Test", "Urgency": 5, ...}');
  
  Result := ask Smart <- 'test input' into TCustomerIntent;
  
  case Result of
    Ok(Intent):
      Assert.Equal(Intent.Urgency, 5);
      Assert.Equal(Intent.Summary, 'Test');
    Err(F):
      Assert.Fail(f'Unexpected failure: {F.Message}');
  end;
end;
```

### 8.3 Fuzzing Structured Extraction

```pascal
test 'Extraction handles malformed JSON';
var
  Result: TOracleResult<TCustomerIntent>;
begin
  // The compiler generates edge cases from the schema
  for MalformedResponse in TCustomerIntent.FuzzCases do
    Smart.QueueResponse(MalformedResponse);
    
    Result := ask Smart <- 'test' into TCustomerIntent;
    
    case Result of
      Ok(_):
        Assert.Fail('Should have failed on malformed input');
      Err(F):
        Assert.Equal(F.Kind, ExtractionFailed);
    end;
  end;
end;
```

### 8.4 Distribution Testing

For testing probabilistic behavior:

```pascal
test 'Sentiment analysis is reasonably accurate';
var
  Correct: Integer;
  Total: Integer;
begin
  Total := 100;
  Correct := 0;
  
  for (Input, Expected) in LoadTestCases('sentiment_tests.csv') do
    Result := ask Smart <- f'Classify sentiment: {Input}' into TSentiment;
    case Result of
      Ok(Sentiment):
        if Sentiment = Expected then
          Inc(Correct);
      Err(_):
        // Ignore failed calls for accuracy measurement
    end;
  end;
  
  // Statistical assertion
  Assert.Proportion(Correct, Total, expected: 0.85, tolerance: 0.05);
end;
```

---

## 9. Error Handling

Liminal uses two complementary error mechanisms:
- **Result types** (`!T`) for expected failures that callers should handle
- **Exceptions** for unexpected failures that propagate up the stack

Oracle operations return Result types because failure is *expected*, not exceptional.

### 9.1 Result Types

Functions that can fail return `!T` (Result type):

```pascal
function SafeDivide(A, B: Integer): !Integer;
begin
  if B = 0 then
    Result := Err('Division by zero')
  else
    Result := Ok(A div B);
end;

// Usage: pattern matching
case SafeDivide(10, X) of
  Ok(Value): WriteLn(f'Result: {Value}');
  Err(Msg): WriteLn(f'Failed: {Msg}');
end;

// Usage: propagate with ?
function Compute(X: Integer): !Integer;
begin
  var Half := SafeDivide(X, 2)?;  // Returns early if Err
  Result := Ok(Half + 1);
end;
```

### 9.2 Oracle Result Types

All oracle operations return `TOracleResult<T>`, which is a Result type with structured failure information:

```pascal
types
  // The failure reasons for oracle operations
  TOracleFailure = record
    Kind: TOracleFailureKind;
    Message: String;
    Details: ?TFailureDetails;  // Additional info depending on Kind
  end;
  
  TOracleFailureKind = (
    NetworkError,        // Connection failed, DNS resolution, etc.
    Timeout,             // Request exceeded time limit
    RateLimited,         // API rate limit hit (Details has RetryAfter)
    ContextOverflow,     // Input exceeds model's context window
    ContentFiltered,     // Content policy violation
    ExtractionFailed,    // Could not extract/validate structured data
    BudgetExceeded,      // Cost limit reached
    ModelRefusal         // Model declined to respond
  );
  
  // Details for specific failure kinds
  TFailureDetails = record
    RetryAfter: ?Integer;           // For RateLimited: seconds to wait
    Field: ?String;                 // For ExtractionFailed: which field
    Constraint: ?String;            // For ExtractionFailed: which constraint
    Value: ?String;                 // For ExtractionFailed: what was received
    TokensUsed: ?Integer;           // For ContextOverflow: how many tokens
    TokenLimit: ?Integer;           // For ContextOverflow: the limit
  end;
  
  // The result type for oracle operations
  TOracleResult<T> = !T with TOracleFailure;
```

### 9.3 Handling Oracle Results

```pascal
var
  Result: TOracleResult<String>;
begin
  Result := ask Smart <- 'What is 2+2?';
  
  case Result of
    Ok(Response):
      WriteLn(Response);
    
    Err(Failure):
      case Failure.Kind of
        NetworkError:
          WriteLn(f'Network failed: {Failure.Message}');
        Timeout:
          WriteLn('Request timed out');
        RateLimited:
          WriteLn(f'Rate limited. Retry after {Failure.Details.RetryAfter}s');
        ExtractionFailed:
          WriteLn(f'Field {Failure.Details.Field} failed: {Failure.Details.Constraint}');
        else
          WriteLn(f'Oracle failed: {Failure.Message}');
      end;
  end;
end;
```

### 9.4 Defensive Patterns

```pascal
// Pattern 1: Fallback chain with 'else'
function GetAnswer(Query: String): String;
begin
  Result := ask Smart <- Query
    else ask Fast <- Query           // If Smart fails, try Fast
    else 'I could not generate a response.';  // Final fallback (always succeeds)
end;

// Pattern 2: Timeout with fallback
Result := ask Smart <- Query
  timeout 5s
  else 'Request timed out';

// Pattern 3: Unwrap with default (for simple cases)
var Response := (ask Smart <- Query).UnwrapOr('Default response');

// Pattern 4: Full pattern matching (shown above in 9.3)
```

### 9.5 Exceptions

Exceptions are reserved for programming errors and truly unexpected conditions:

```pascal
types
  // Standard exceptions (not oracle-related)
  Exception = class
    Message: String;
  end;
  
  EArgumentError = class(Exception);    // Invalid argument passed
  EIndexOutOfBounds = class(Exception); // Array/string index invalid
  EFileNotFound = class(Exception);     // File system errors
  EInvalidState = class(Exception);     // Logic errors
```

Use `try...except` only for these:

```pascal
try
  Content := ReadFile('config.json');
  Config := ParseJSON(Content, TConfig);
except
  on E: EFileNotFound do
    WriteLn('Config file missing, using defaults');
    Config := DefaultConfig;
  on E: Exception do
    WriteLn(f'Unexpected error: {E.Message}');
    raise;  // Re-raise
end;
```

**Design rationale:** Oracle failures are *expected*—networks fail, rate limits hit, models refuse. Making them Result types forces explicit handling. Exceptions are for bugs and unexpected conditions that shouldn't happen in correct code.

---

## 10. The Standard Library

### 10.1 Core Modules

```pascal
uses
  System,       // Basic I/O, environment, process
  Strings,      // String manipulation
  Collections,  // Lists, maps, sets
  Files,        // File system operations
  JSON,         // JSON parsing and generation
  HTTP,         // HTTP client
  Regex,        // Regular expressions
  Time,         // Date, time, duration
  Math,         // Mathematical functions
  Crypto;       // Hashing, encoding
```

### 10.2 AI-Specific Modules

```pascal
uses
  Oracles,      // Oracle types and configuration
  Vectors,      // Vector operations, indexing
  Chunking,     // Text chunking strategies
  Schemas,      // Schema generation and validation
  Prompts,      // Prompt templates and composition
  Documents;    // Document loading (PDF, DOCX, etc.)
```

### 10.3 Document Processing

```pascal
uses
  Documents;

var
  Doc: TDocument;
  Chunks: array of TChunk;
begin
  // Load with explicit format
  Doc := Document.Load('manual.pdf', format: PDF);
  
  // Access metadata
  WriteLn(f'Pages: {Doc.PageCount}');
  WriteLn(f'Size: {Doc.CharCount} characters');
  
  // Chunk with explicit strategy
  Chunks := Doc.Chunk(
    strategy: SentenceBoundary,
    targetSize: 500,    // Target tokens per chunk
    overlap: 50         // Overlap tokens
  );
  
  for Chunk in Chunks do
    WriteLn(f'Chunk {Chunk.Index}: {Chunk.Text[0..50]}...');
end;
```

### 10.4 Vector Index

```pascal
uses
  Vectors;

var
  Index: TVectorIndex;
  Results: array of TSearchResult;
begin
  // Create index with explicit configuration
  Index := VectorIndex.Create(
    oracle: Embedder,
    metric: Cosine,         // or: Euclidean, DotProduct
    storage: InMemory       // or: SQLite('index.db'), HNSW('index.hnsw')
  );
  
  // Add documents
  for Chunk in Chunks do
    Index.Add(
      id: Chunk.Id,
      text: Chunk.Text,
      metadata: {page: Chunk.PageNumber}
    );
  
  // Search
  Results := Index.Search(
    query: 'How do I reset my password?',
    topK: 5,
    threshold: 0.7
  );
  
  for R in Results do
    WriteLn(f'{R.Score}: {R.Text}');
end;
```

---

## 11. Complete Example: RAG CLI Tool

```pascal
program DocAssistant;

uses
  System, Files, Documents, Vectors, JSON;

config
  OracleMode = env.ORACLE_MODE ?? 'live';

oracles
  Embedder: EmbedOracle = 'text-embedding-3-small' via OpenAI(env.OPENAI_API_KEY);
  Chat: TextOracle = 'gpt-4o' via OpenAI(env.OPENAI_API_KEY);

types
  schema TAnswer
    Response: String[1..2000]
      describe 'Direct answer to the question based on the provided context';
    Confidence: Integer[1..100]
      describe 'Confidence percentage that this answer is correct and complete';
    SourceChunks: array[0..5] of String
      describe 'Quote the specific passages that support this answer';
  end;

var
  Index: TVectorIndex;
  DocPath: String;
  Query: String;
  Context: String;
  
function BuildContext(Results: array of TSearchResult): String;
var
  Builder: TStringBuilder;
begin
  Builder := StringBuilder.Create;
  for R in Results do
    Builder.AppendLine(f'[Source, relevance {R.Score:.2f}]');
    Builder.AppendLine(R.Text);
    Builder.AppendLine;
  end;
  Result := Builder.ToString;
end;

begin
  // Parse arguments
  if ParamCount < 1 then
    Halt('Usage: docassistant <document.pdf>');
  
  DocPath := ParamStr(1);
  
  // Load and index document
  WriteLn(f'Loading {DocPath}...');
  
  var Doc := Document.Load(DocPath);
  var Chunks := Doc.Chunk(strategy: Paragraph, targetSize: 400, overlap: 50);
  
  WriteLn(f'Indexing {Length(Chunks)} chunks...');
  
  Index := VectorIndex.Create(Embedder, storage: InMemory);
  
  within budget $0.50 do  // Limit indexing cost
    for Chunk in Chunks do
      Index.Add(Chunk.Id, Chunk.Text);
  on exceeded do
    WriteLn('Warning: Document too large, index is partial');
  end;
  
  WriteLn('Ready. Type your questions (empty line to quit).');
  WriteLn;
  
  // Main loop
  loop
    Write('> ');
    ReadLn(Query);
    
    if Query.IsEmpty then
      break;
    
    // Search for relevant context
    var Results := Index.Search(Query, topK: 5, threshold: 0.6);
    
    if Results.IsEmpty then
      WriteLn('No relevant sections found in the document.');
      continue;
    end;
    
    Context := BuildContext(Results);
    
    // Ask with structured extraction
    var AnswerResult := consult Chat from
      f'Based on the following context from a document, answer the user question.\n\n' +
      f'Context:\n{Context}\n\n' +
      f'Question: {Query}'
    into TAnswer
    with
      attempts: 2,
      timeout: 30s,
      budget: $0.05
    on failure(F: TOracleFailure)
      case F.Kind of
        ExtractionFailed:
          retry with hint f'Field {F.Details.Field} is invalid: {F.Details.Constraint}';
        Timeout:
          yield Err(F);
        BudgetExceeded:
          yield Err(F);
        else
          retry;
      end;
    end;
    
    case AnswerResult of
      Err(F):
        WriteLn(f'Could not generate answer: {F.Message}');
        continue;
      Ok(Answer):
        // Display answer
        WriteLn;
        WriteLn(Answer.Response);
        WriteLn;
        WriteLn(f'Confidence: {Answer.Confidence}%');
        
        if Answer.Confidence < 70 then
          WriteLn('Note: Low confidence. The document may not contain a complete answer.');
        end;
    end;
    
    WriteLn;
  end;
  
  WriteLn('Goodbye.');
end.
```

---

## 12. Compilation and Execution

### 12.1 Compiler Invocation

```bash
# Compile to executable
$ liminal compile program.lim -o program

# Compile with cost estimation
$ liminal compile program.lim --estimate-costs

# Type check only
$ liminal check program.lim

# Run directly (JIT)
$ liminal run program.lim

# Generate JSON schemas for all schema types
$ liminal schemas program.lim -o schemas/
```

### 12.2 Compiler Errors

The compiler produces clear errors for probabilistic code:

```
program.lim:42:5: error: type mismatch in oracle extraction
  |
42|   Intent := ask Smart <- Query into TCustomerIntent;
  |             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  |
  = Schema field 'Urgency' has type Integer[1..10]
  = But record TCustomerIntent.Urgency is declared as Integer (unbounded)
  = 
  = hint: Add range constraint to the record field:
  =   Urgency: Integer[1..10];
```

```
program.lim:58:3: warning: oracle call without error handling
  |
58|   Result := ask Smart <- UserInput;
  |   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  |
  = This oracle call can fail with: NetworkError, Timeout, ContentFiltered
  = Consider using 'consult' or handling the result (case/else/UnwrapOr)
  =
  = hint: Add a fallback:
  =   Result := ask Smart <- UserInput else 'Default response';
```

### 12.3 Runtime Configuration

Oracles can be reconfigured at runtime via environment or config file:

```ini
; liminal.ini
[oracles.Smart]
provider = AzureOpenAI
endpoint = https://mycompany.openai.azure.com
deployment = gpt-4-turbo
api_key = ${AZURE_OPENAI_KEY}

[oracles.Fast]  
provider = Ollama
endpoint = http://localhost:11434
model = llama3:8b

[budgets]
default_per_query = 0.05
max_per_session = 5.00
```

---

## 13. Future Directions

### 13.1 Planned Features

- **Agents**: First-class support for tool-using agents with explicit action spaces
- **Streaming Extraction**: Parse structured data as it streams in
- **Parallel Oracles**: Fan-out queries with built-in aggregation helpers
- **Fine-tuning Integration**: Train and deploy custom models from Liminal
- **Provenance Tracking**: Record the full chain of reasoning for audit

### 13.2 Non-Goals

Liminal explicitly does not aim to:

- Hide the probabilistic nature of LLMs behind deterministic syntax
- Compete with Python for ML research or model training
- Provide a "no-code" or visual programming experience
- Abstract away the differences between model providers

---

## Appendix A: Grammar Summary

```ebnf
(* ===== Program Structure ===== *)

program       = 'program' identifier ';' 
                { top_level_item }
                'begin' statements 'end' '.' ;

top_level_item = uses_clause
               | config_block
               | types_block  
               | oracles_block
               | var_block
               | function_decl ;

uses_clause   = 'uses' identifier { ',' identifier } ';' ;

config_block  = 'config' { config_item } ;

config_item   = identifier '=' expression ';' ;

types_block   = 'types' { type_decl | schema_decl } ;

oracles_block = 'oracles' { oracle_decl } ;

var_block     = 'var' { var_decl } ;


(* ===== Type Declarations ===== *)

type_decl     = identifier '=' type_def ';' ;

type_def      = primitive_type
              | constrained_type
              | array_type
              | tuple_type
              | record_type
              | enum_type
              | optional_type
              | result_type ;

primitive_type = 'Integer' | 'Real' | 'Boolean' | 'Char' | 'String' | 'Byte' | 'Bytes' ;

constrained_type = 'Integer' '[' range ']'
                 | 'String' '[' range ']'
                 | 'String' 'matching' string_literal ;

array_type    = 'array' 'of' type_def                          (* dynamic *)
              | 'array' '[' integer { ',' integer } ']' 'of' type_def   (* fixed size *)
              | 'array' '[' range ']' 'of' type_def ;          (* constrained length *)

tuple_type    = 'tuple' '<' type_def { ',' type_def } '>' ;

record_type   = 'record' { field_decl ';' } 'end' ;

schema_decl   = 'schema' identifier { schema_field } 'end' ';' ;

schema_field  = identifier ':' type_def [ 'describe' string_literal ] ';' ;

enum_type     = '(' identifier { ',' identifier } ')' ;

optional_type = '?' type_def ;

result_type   = '!' type_def [ 'with' type_def ] ;

range         = expression '..' expression ;

field_decl    = identifier ':' type_def ;


(* ===== Oracle Declarations ===== *)

oracle_decl   = identifier ':' oracle_type '=' string_literal 'via' provider_spec ';' ;

oracle_type   = 'TextOracle' | 'EmbedOracle' | 'VisionOracle' | 'StructuredOracle' ;

provider_spec = identifier [ '(' argument_list ')' ] ;


(* ===== Variable Declarations ===== *)

var_decl      = identifier { ',' identifier } ':' type_def [ '=' expression ] ';' ;


(* ===== Function Declarations ===== *)

function_decl = 'function' identifier '(' [ param_list ] ')' ':' type_def ';'
                [ var_block ]
                'begin' statements 'end' ';' ;

param_list    = param_decl { ';' param_decl } ;

param_decl    = identifier { ',' identifier } ':' type_def ;


(* ===== Statements ===== *)

statements    = { statement ';' } ;

statement     = assignment
              | if_stmt
              | case_stmt
              | for_stmt
              | while_stmt
              | repeat_stmt
              | loop_stmt
              | parallel_stmt
              | break_stmt
              | continue_stmt
              | return_stmt
              | try_stmt
              | consult_stmt
              | budget_stmt
              | stream_stmt
              | expression ;

assignment    = (lvalue | tuple_lvalue) ':=' expression ;

lvalue        = identifier [ '[' index_or_slice { ',' index_or_slice } ']' ]
              | lvalue '.' identifier ;

tuple_lvalue  = '(' lvalue { ',' lvalue } ')' ;

index_or_slice = expression | slice_expr ;

slice_expr    = [ expression ] '..' [ expression ] ;

if_stmt       = 'if' expression 'then' statement
                { 'else' 'if' expression 'then' statement }
                [ 'else' statement ] ;

case_stmt     = 'case' expression 'of'
                { case_branch }
                [ 'else' statement ]
                'end' ;

case_branch   = pattern ':' statement ';' ;

pattern       = identifier [ '(' identifier { ',' identifier } ')' ]
              | literal ;

for_stmt      = 'for' identifier ':=' expression 'to' expression 'do' statement
              | 'for' identifier 'in' expression 'do' statement ;

while_stmt    = 'while' expression 'do' statement ;

repeat_stmt   = 'repeat' statements 'until' expression ;

loop_stmt     = 'loop' statements 'end' ;

parallel_stmt = 'parallel' statements 'end' ;

break_stmt    = 'break' ;

continue_stmt = 'continue' ;

return_stmt   = 'Result' ':=' expression ;

try_stmt      = 'try' statements
                'except' { 'on' identifier ':' type_def 'do' statement }
                'end' ;


(* ===== Oracle Statements ===== *)

consult_stmt  = 'consult' identifier 'from' expression
                'into' type_def
                'with' consult_options
                'on' 'failure' '(' identifier ':' 'TOracleFailure' ')'
                  consult_case
                'end' ;

consult_case  = 'case' expression 'of'
                { consult_case_branch }
                [ 'else' consult_action_block ]
                'end' ;

consult_case_branch = pattern ':' consult_action_block ;

consult_options = consult_option { ',' consult_option } ;

consult_option  = 'attempts' ':' integer
                | 'timeout' ':' duration
                | 'budget' ':' money_literal ;

consult_action_block = { consult_action ';' } ;

consult_action = failure_action | statement ;

failure_action = 'retry' [ 'with' 'hint' expression ]
               | 'wait' duration
               | 'yield' expression
               | 'yield' 'Err' '(' expression ')' ;

budget_stmt   = 'within' 'budget' money_literal 'do' 
                statements
                'on' 'exceeded' 'do' statement
                'end' ;

stream_stmt   = 'stream' identifier '<-' expression 'do' '|' identifier '|'
                statements
                'end' ;


(* ===== Expressions ===== *)

expression    = or_expr [ '?' expression ':' expression ] ;  (* ternary *)

or_expr       = and_expr { 'or' and_expr } ;

and_expr      = compare_expr { 'and' compare_expr } ;

compare_expr  = add_expr { compare_op add_expr } ;

compare_op    = '=' | '<>' | '<' | '>' | '<=' | '>=' ;

add_expr      = mul_expr { add_op mul_expr } ;

add_op        = '+' | '-' ;

mul_expr      = unary_expr { mul_op unary_expr } ;

mul_op        = '*' | '/' | 'div' | 'mod' ;

unary_expr    = [ 'not' | '-' ] postfix_expr ;

postfix_expr  = primary_expr { postfix_op } ;

postfix_op    = '.' identifier                          (* field access *)
              | '[' index_or_slice { ',' index_or_slice } ']'   (* indexing/slicing *)
              | '(' [ argument_list ] ')'               (* call *)
              | '?' ;                                   (* result propagation *)

primary_expr  = literal
              | identifier
              | '(' expression ')'
              | array_literal
              | tuple_literal
              | record_literal
              | ask_expr
              | embed_expr
              | context_expr
              | f_string ;

ask_expr      = 'ask' identifier '<-' expression
                [ 'into' type_def ]
                [ 'timeout' duration ]
                [ 'else' expression ]
                [ 'with' 'cost' ] ;

embed_expr    = 'embed' identifier '<-' expression ;

context_expr  = 'Context' '.' 'Create' '(' identifier ')'
                { '.' context_method } ;

context_method = 'WithSystem' '(' string_literal ')'
               | 'WithMaxTokens' '(' integer ')'
               | 'OnOverflow' '(' 'strategy' ':' identifier ')' ;

argument_list = expression { ',' expression }
              | named_arg { ',' named_arg } ;

named_arg     = identifier ':' expression ;


(* ===== Literals ===== *)

literal       = integer_literal
              | real_literal
              | string_literal
              | bytes_literal
              | char_literal
              | boolean_literal
              | duration
              | money_literal
              | 'Nothing' ;

integer_literal = digit { digit } ;

integer       = integer_literal ;

real_literal  = digit { digit } '.' digit { digit } ;

string_literal = "'" { character } "'" ;

bytes_literal = "b" "'" { character } "'" ;

f_string      = 'f' "'" { character | '{' expression '}' } "'" ;

char_literal  = "'" character "'" ;

boolean_literal = 'True' | 'False' ;

duration      = integer ('s' | 'ms' | 'm' | 'h') ;

money_literal = '$' real_literal ;

array_literal = '[' [ expression { ',' expression } ] ']' ;

tuple_literal = '(' expression ',' expression { ',' expression } ')' ;

record_literal = '{' [ field_init { ',' field_init } ] '}' ;

field_init    = identifier ':' expression ;


(* ===== Lexical Elements ===== *)

identifier    = letter { letter | digit | '_' } ;

letter        = 'a'..'z' | 'A'..'Z' ;

digit         = '0'..'9' ;

character     = (* any UTF-8 character except quote, or escaped *) ;
```

**Grammar Notes:**

1. Semicolons terminate statements and declarations, not separate them.
2. Schema fields use semicolons like record fields.
3. The `ask` expression's `else` clause provides a fallback value if the oracle call fails.
4. The `?` postfix operator propagates errors in Result types (early return on Err).
5. Duration literals: `5s` (seconds), `100ms` (milliseconds), `2m` (minutes), `1h` (hours).
6. Money literals: `$0.10`, `$5.00` for budget specifications.
7. Tuples use `tuple<...>` in types and `(a, b)` as literals and destructuring targets.
8. In f-strings, literal `{` and `}` are escaped as `{{` and `}}`.

---

## Appendix B: Comparison with Alternatives

| Feature | Liminal | Python + Libraries | LMQL | TypeScript + Zod |
|---------|---------|-------------------|------|------------------|
| Deterministic code performance | Native | Interpreted | Interpreted | JIT |
| Type-checked LLM outputs | Compile-time | Runtime | Runtime | Runtime |
| Cost visibility | Language-level | Manual | None | Manual |
| Retry with hints | Built-in | Library | None | Library |
| Testing support | First-class | Library | Manual | Library |
| Learning curve (from Pascal) | Low | Medium | Medium | Medium |

---

*End of Specification*