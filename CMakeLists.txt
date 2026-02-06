cmake_minimum_required(VERSION 3.16)
project(liminal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers in debug builds" ON)
option(ENABLE_FUZZING "Enable fuzzing targets" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g3)
  add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

if(ENABLE_COVERAGE)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage)
    add_link_options(--coverage)
  endif()
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
