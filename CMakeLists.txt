cmake_minimum_required(VERSION 3.20)
project(axiom VERSION 0.1.0 LANGUAGES CXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(AXIOM_BUILD_TESTS "Build the test suite" ON)
option(AXIOM_BUILD_EXAMPLES "Build example programs" ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find LLVM
find_package(LLVM CONFIG REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Get LLVM libraries - need X86 for native target on Windows
llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    irreader
    native
    target
)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Axiom frontend library (no LLVM dependency)
add_library(axiom_frontend STATIC
    src/lexer/token.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/semantic/types.cpp
    src/semantic/symbol_table.cpp
    src/semantic/type_checker.cpp
)

# Axiom codegen library (LLVM dependency)
add_library(axiom_codegen STATIC
    src/codegen/codegen.cpp
)
target_link_libraries(axiom_codegen PUBLIC axiom_frontend ${LLVM_LIBS})

# Axiom REPL library
add_library(axiom_repl STATIC
    src/repl/repl.cpp
)
target_link_libraries(axiom_repl PUBLIC axiom_codegen)

# Axiom compiler executable
add_executable(axiom src/main.cpp)
target_link_libraries(axiom PRIVATE axiom_repl)

# Tests
if(AXIOM_BUILD_TESTS)
    enable_testing()
    
    # Lexer tests
    add_executable(lexer_test tests/lexer_test.cpp)
    target_link_libraries(lexer_test PRIVATE axiom_frontend)
    add_test(NAME LexerTest COMMAND lexer_test)
    
    # Parser tests
    add_executable(parser_test tests/parser_test.cpp)
    target_link_libraries(parser_test PRIVATE axiom_frontend)
    add_test(NAME ParserTest COMMAND parser_test)
    
    # Semantic tests
    add_executable(semantic_test tests/semantic_test.cpp)
    target_link_libraries(semantic_test PRIVATE axiom_frontend)
    add_test(NAME SemanticTest COMMAND semantic_test)
    
    # Codegen tests
    add_executable(codegen_test tests/codegen_test.cpp)
    target_link_libraries(codegen_test PRIVATE axiom_codegen)
    add_test(NAME CodegenTest COMMAND codegen_test)
    
    # Stdlib tests
    add_executable(stdlib_test tests/stdlib_test.cpp)
    add_test(NAME StdlibTest COMMAND stdlib_test)
endif()

# Install rules
install(TARGETS axiom RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# Print configuration summary
message(STATUS "")
message(STATUS "=== Axiom Configuration ===")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests:  ${AXIOM_BUILD_TESTS}")
message(STATUS "  LLVM Version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "")

