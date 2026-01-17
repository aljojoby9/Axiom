/**
 * @file codegen_test.cpp
 * @brief Unit tests for Axiom code generation
 */

#include <iostream>
#include <cassert>
#include "codegen/codegen.hpp"
#include "semantic/type_checker.hpp"
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"

using namespace axiom;
using namespace axiom::codegen;
using namespace axiom::semantic;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "... "; \
    test_##name(); \
    std::cout << "PASSED\n"; \
} while(0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        std::cerr << "\nAssertion failed: " << #x << "\n"; \
        std::exit(1); \
    } \
} while(0)

#define ASSERT_NO_ERRORS(obj) do { \
    if (obj.has_errors()) { \
        std::cerr << "\nErrors:\n"; \
        for (const auto& err : obj.errors()) { \
            std::cerr << "  " << err.message << "\n"; \
        } \
        std::exit(1); \
    } \
} while(0)

ast::Program parse_and_check(const char* source, TypeChecker& checker) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parse();
    checker.check(program);
    return program;
}

// === Initialization Test ===

TEST(llvm_init) {
    // Initialize LLVM
    initialize_llvm();
    ASSERT_TRUE(true);  // Just checking it doesn't crash
}

// === Simple Function Tests ===

TEST(empty_function) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn empty():
    return
)", checker);
    
    CodeGenerator codegen("test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
}

TEST(return_integer) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn answer() -> i64:
    return 42
)", checker);
    
    CodeGenerator codegen("test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
}

TEST(simple_arithmetic) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn add(a: i64, b: i64) -> i64:
    return a + b
)", checker);
    
    CodeGenerator codegen("test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
}

TEST(variable_declaration) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn test() -> i64:
    let x = 10
    let y = 20
    return x + y
)", checker);
    
    CodeGenerator codegen("test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
}

TEST(if_statement) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn max(a: i64, b: i64) -> i64:
    if a > b:
        return a
    else:
        return b
)", checker);
    
    CodeGenerator codegen("test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
}

TEST(while_loop) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn countdown(n: i64) -> i64:
    var count = n
    while count > 0:
        count = count - 1
    return count
)", checker);
    
    CodeGenerator codegen("test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
}

TEST(function_call) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn double(x: i64) -> i64:
    return x + x

fn test() -> i64:
    return double(21)
)", checker);
    
    CodeGenerator codegen("test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
}

TEST(multiple_operations) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn compute(a: i64, b: i64) -> i64:
    let sum = a + b
    let diff = a - b
    let prod = a * b
    return sum + diff + prod
)", checker);
    
    CodeGenerator codegen("test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
}

TEST(comparison_ops) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn compare(a: i64, b: i64) -> bool:
    return a < b
)", checker);
    
    CodeGenerator codegen("test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
}

TEST(print_ir) {
    TypeChecker checker;
    auto program = parse_and_check(R"(
fn fibonacci(n: i64) -> i64:
    if n < 2:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)
)", checker);
    
    CodeGenerator codegen("fibonacci_test");
    ASSERT_TRUE(codegen.generate(program, checker));
    ASSERT_NO_ERRORS(codegen);
    
    // Optionally print IR for debugging
    // codegen.dump_ir();
}

int main() {
    std::cout << "=== Axiom Codegen Tests ===\n\n";
    
    RUN_TEST(llvm_init);
    RUN_TEST(empty_function);
    RUN_TEST(return_integer);
    RUN_TEST(simple_arithmetic);
    RUN_TEST(variable_declaration);
    RUN_TEST(if_statement);
    RUN_TEST(while_loop);
    RUN_TEST(function_call);
    RUN_TEST(multiple_operations);
    RUN_TEST(comparison_ops);
    RUN_TEST(print_ir);
    
    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
