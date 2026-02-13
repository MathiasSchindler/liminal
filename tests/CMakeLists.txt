add_library(test_harness
  test_harness.c
)

target_include_directories(test_harness PUBLIC ${PROJECT_SOURCE_DIR}/include)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(test_harness PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()

add_executable(liminal_cli_tests
  test_cli.c
)

target_link_libraries(liminal_cli_tests PRIVATE test_harness liminal_lib)
target_compile_definitions(liminal_cli_tests PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")

add_executable(liminal_lexer_tests
  test_lexer.c
)

target_link_libraries(liminal_lexer_tests PRIVATE test_harness liminal_lib)
target_compile_definitions(liminal_lexer_tests PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")

add_test(NAME liminal_cli_tests COMMAND liminal_cli_tests)

# Examples smoke
add_test(NAME examples_hello COMMAND ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/01_hello.lim)
set_tests_properties(examples_hello PROPERTIES TIMEOUT 5)
add_test(NAME examples_add COMMAND bash -lc "printf '3\n4\n' | ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/02_add.lim")
set_tests_properties(examples_add PROPERTIES TIMEOUT 5)
add_test(NAME examples_ask_basic COMMAND bash -lc "LIMINAL_ORACLE_PROVIDER=mock LIMINAL_ORACLE_MODE=replay LIMINAL_ORACLE_RECORDING=${PROJECT_SOURCE_DIR}/tests/recordings/examples.jsonl ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/04_ask_basic.lim")
set_tests_properties(examples_ask_basic PROPERTIES TIMEOUT 5)
add_test(NAME examples_ask_with_cost COMMAND bash -lc "LIMINAL_ORACLE_PROVIDER=mock LIMINAL_ORACLE_MODE=replay LIMINAL_ORACLE_RECORDING=${PROJECT_SOURCE_DIR}/tests/recordings/examples.jsonl ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/05_ask_with_cost.lim")
set_tests_properties(examples_ask_with_cost PROPERTIES TIMEOUT 5)
add_test(NAME examples_ask_into COMMAND bash -lc "LIMINAL_ORACLE_PROVIDER=mock LIMINAL_ORACLE_MODE=replay LIMINAL_ORACLE_RECORDING=${PROJECT_SOURCE_DIR}/tests/recordings/examples.jsonl ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/07_ask_into.lim")
set_tests_properties(examples_ask_into PROPERTIES TIMEOUT 5)
add_test(NAME examples_mixed COMMAND bash -lc "LIMINAL_ORACLE_PROVIDER=mock LIMINAL_ORACLE_MODE=replay LIMINAL_ORACLE_RECORDING=${PROJECT_SOURCE_DIR}/tests/recordings/examples.jsonl ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/08_mixed.lim")
set_tests_properties(examples_mixed PROPERTIES TIMEOUT 5)
add_test(NAME examples_consult_retry COMMAND bash -lc "LIMINAL_ORACLE_PROVIDER=mock LIMINAL_ORACLE_MODE=replay LIMINAL_ORACLE_RECORDING=${PROJECT_SOURCE_DIR}/tests/recordings/examples.jsonl ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/09_consult_retry.lim")
set_tests_properties(examples_consult_retry PROPERTIES TIMEOUT 5)

option(ENABLE_OPUS_SMOKE_TESTS "Enable Opus smoke tests (t* and c* only)" OFF)
option(ENABLE_OPUS_BENCHMARK_TESTS "Enable Opus benchmark-focused tests" OFF)

if(ENABLE_OPUS_SMOKE_TESTS)
  # Opus smoke (deterministic focus): atomic t* and combined c* only.
  # Advanced oracle programs o* are intentionally excluded for now.
  set(OPUS_ATOMIC_PROGRAMS
    t01_hello
    t02_int_vars
    t03_int_arith
    t04_real_arith
    t05_booleans
    t06_strings
    t07_comparisons
    t08_if_else
    t09_if_chain
    t10_for_loop
    t11_while
    t12_repeat
    t13_function
    t14_func_params
    t15_recursion
    t16_record
    t17_array
    t18_enum
    t19_case_int
    t20_fstring
    t21_local_vars
    t22_begin_end
    t23_writeln
    t24_nested_calls
    t25_optional
    t26_file_roundtrip_guard
    t27_record_patch
    t28_schema_validation_path
  )

  foreach(opus_prog IN LISTS OPUS_ATOMIC_PROGRAMS)
    add_test(
      NAME opus_${opus_prog}
      COMMAND ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/opus/${opus_prog}.lim
    )
    set_tests_properties(opus_${opus_prog} PROPERTIES TIMEOUT 5 LABELS "opus;opus_atomic")
  endforeach()

  set(OPUS_COMBINED_PROGRAMS
    c01_fibonacci
    c02_person_greet
    c03_traffic_light
    c04_array_ops
    c05_result_types
    c06_data_table
    c07_gcd_lcm
    c08_constraints
    c09_invoice_batch
    c10_shift_alerting
    c11_ticket_triage_extract
  )

  foreach(opus_prog IN LISTS OPUS_COMBINED_PROGRAMS)
    add_test(
      NAME opus_${opus_prog}
      COMMAND ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/opus/${opus_prog}.lim
    )
    set_tests_properties(opus_${opus_prog} PROPERTIES TIMEOUT 5 LABELS "opus;opus_combined")
  endforeach()
endif()

if(ENABLE_OPUS_BENCHMARK_TESTS)
  set(OPUS_BENCH_PROGRAMS
    t29_bench_int_hotloop
    t30_bench_function_calls
    c12_bench_event_aggregation
    c13_bench_rule_routing
  )

  foreach(opus_prog IN LISTS OPUS_BENCH_PROGRAMS)
    add_test(
      NAME opus_bench_${opus_prog}
      COMMAND ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/opus/${opus_prog}.lim
    )
    set_tests_properties(opus_bench_${opus_prog} PROPERTIES TIMEOUT 20 LABELS "opus_bench")
  endforeach()
endif()
add_test(NAME liminal_lexer_tests COMMAND liminal_lexer_tests)

add_executable(liminal_parser_tests
  test_parser.c
)

target_link_libraries(liminal_parser_tests PRIVATE test_harness liminal_lib)
target_compile_definitions(liminal_parser_tests PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")

add_test(NAME liminal_parser_tests COMMAND liminal_parser_tests)
set_tests_properties(liminal_parser_tests PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")

add_executable(liminal_typecheck_tests
  test_typecheck.c
)

target_link_libraries(liminal_typecheck_tests PRIVATE test_harness liminal_lib)

add_test(NAME liminal_typecheck_tests COMMAND liminal_typecheck_tests)

add_executable(liminal_runtime_tests
  test_runtime.c
)

target_link_libraries(liminal_runtime_tests PRIVATE test_harness liminal_lib)
add_test(NAME liminal_runtime_tests COMMAND liminal_runtime_tests)

add_executable(liminal_schema_tests
  test_schema_emit.c
)

target_link_libraries(liminal_schema_tests PRIVATE test_harness liminal_lib)
target_compile_definitions(liminal_schema_tests PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME liminal_schema_tests COMMAND liminal_schema_tests)
set_tests_properties(liminal_parser_tests PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")

add_executable(liminal_ir_tests
  test_ir.c
)

target_link_libraries(liminal_ir_tests PRIVATE test_harness liminal_lib)
target_compile_definitions(liminal_ir_tests PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME liminal_ir_tests COMMAND liminal_ir_tests)

add_executable(liminal_exec_tests
  test_exec.c
)

target_link_libraries(liminal_exec_tests PRIVATE test_harness liminal_lib)
target_compile_definitions(liminal_exec_tests PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME liminal_exec_tests COMMAND liminal_exec_tests)

add_executable(liminal_oracle_tests
  test_oracles.c
)

add_executable(liminal_ask_tests
  test_ask.c
)
add_executable(liminal_consult_tests
  test_consult.c
)

target_link_libraries(liminal_oracle_tests PRIVATE test_harness liminal_lib)
add_test(NAME liminal_oracle_tests COMMAND liminal_oracle_tests)
set_tests_properties(liminal_oracle_tests PROPERTIES TIMEOUT 5)

target_link_libraries(liminal_ask_tests PRIVATE test_harness liminal_lib)
target_compile_definitions(liminal_ask_tests PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME liminal_ask_tests COMMAND liminal_ask_tests)
set_tests_properties(liminal_ask_tests PROPERTIES TIMEOUT 5 ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")

target_link_libraries(liminal_consult_tests PRIVATE test_harness liminal_lib)
target_compile_definitions(liminal_consult_tests PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME liminal_consult_tests COMMAND liminal_consult_tests)
set_tests_properties(liminal_consult_tests PROPERTIES TIMEOUT 5 ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")

add_test(NAME live_examples_ask_basic COMMAND bash -lc "if [ -z \"$LIMINAL_LIVE\" ]; then echo 'SKIP: set LIMINAL_LIVE=1'; exit 125; fi; LIMINAL_ORACLE_PROVIDER=ollama ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/04_ask_basic.lim")
set_tests_properties(live_examples_ask_basic PROPERTIES TIMEOUT 20 LABELS "live" SKIP_RETURN_CODE 125)
add_test(NAME live_examples_ask_into COMMAND bash -lc "if [ -z \"$LIMINAL_LIVE\" ]; then echo 'SKIP: set LIMINAL_LIVE=1'; exit 125; fi; LIMINAL_ORACLE_PROVIDER=ollama ${PROJECT_BINARY_DIR}/src/liminal run ${PROJECT_SOURCE_DIR}/examples/07_ask_into.lim")
set_tests_properties(live_examples_ask_into PROPERTIES TIMEOUT 20 LABELS "live" SKIP_RETURN_CODE 125)

if(ENABLE_FUZZING)
  add_executable(lexer_fuzz
    fuzz_lexer.c
  )
  target_link_libraries(lexer_fuzz PRIVATE liminal_lib)
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(lexer_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
    target_link_options(lexer_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
  endif()
endif()
