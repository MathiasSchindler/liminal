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
