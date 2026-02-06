set(LIMINAL_SOURCES
  liminal.c
  cli.c
  lexer.c
  ast.c
  parser.c
  types.c
  symtab.c
  typecheck.c
  runtime.c
  schema.c
  ir.c
  exec.c
  oracles.c
  oracle_mock.c
  oracle_record.c
  oracle_ollama.c
  sha256.c
  main.c
)

add_library(liminal_lib
  liminal.c
  cli.c
  lexer.c
  ast.c
  parser.c
  types.c
  symtab.c
  typecheck.c
  runtime.c
  schema.c
  ir.c
  exec.c
  oracles.c
  oracle_mock.c
  oracle_record.c
  oracle_ollama.c
  sha256.c
)

target_include_directories(liminal_lib
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

add_executable(liminal main.c)
target_link_libraries(liminal PRIVATE liminal_lib)

# Strict warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(liminal_lib PRIVATE -Wall -Wextra -Werror -Wpedantic)
  target_compile_options(liminal PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()
