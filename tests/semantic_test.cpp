/**
 * @file semantic_test.cpp
 * @brief Unit tests for Axiom semantic analysis
 */

#include <iostream>
#include <cassert>
#include "semantic/type_checker.hpp"
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"

using namespace axiom;
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

#define ASSERT_NO_ERRORS(checker) do { \
    if (checker.has_errors()) { \
        std::cerr << "\nSemantic errors:\n"; \
        for (const auto& err : checker.errors()) { \
            std::cerr << "  " << err.location.line << ":" << err.location.column \
                      << ": " << err.message << "\n"; \
        } \
        std::exit(1); \
    } \
} while(0)

#define EXPECT_ERROR(checker) do { \
    if (!checker.has_errors()) { \
        std::cerr << "\nExpected error but got none\n"; \
        std::exit(1); \
    } \
} while(0)

ast::Program parse(const char* source) {
    Lexer lexer(source);
    Parser parser(lexer);
    return parser.parse();
}

// === Type System Tests ===

TEST(primitive_types) {
    ASSERT_TRUE(i32_type()->is_integer());
    ASSERT_TRUE(f64_type()->is_float());
    ASSERT_TRUE(bool_type()->kind == TypeKind::Bool);
    ASSERT_TRUE(string_type()->kind == TypeKind::String);
}

TEST(type_equality) {
    ASSERT_TRUE(i32_type()->equals(*i32_type()));
    ASSERT_TRUE(!i32_type()->equals(*i64_type()));
    
    auto list1 = std::make_shared<ListType>(i32_type());
    auto list2 = std::make_shared<ListType>(i32_type());
    auto list3 = std::make_shared<ListType>(i64_type());
    ASSERT_TRUE(list1->equals(*list2));
    ASSERT_TRUE(!list1->equals(*list3));
}

TEST(type_to_string) {
    ASSERT_TRUE(i32_type()->to_string() == "i32");
    ASSERT_TRUE(string_type()->to_string() == "str");
    
    auto list = std::make_shared<ListType>(i32_type());
    ASSERT_TRUE(list->to_string() == "List[i32]");
    
    auto dict = std::make_shared<DictType>(string_type(), i32_type());
    ASSERT_TRUE(dict->to_string() == "Dict[str, i32]");
}

TEST(type_assignability) {
    ASSERT_TRUE(is_assignable(i32_type(), i32_type()));
    ASSERT_TRUE(is_assignable(i32_type(), i64_type()));
    ASSERT_TRUE(is_assignable(i32_type(), f64_type()));
    ASSERT_TRUE(is_assignable(never_type(), i32_type()));
}

// === Symbol Table Tests ===

TEST(symbol_table_basic) {
    SymbolTable st;
    
    ASSERT_TRUE(st.define("x", SymbolKind::Variable, i32_type()));
    ASSERT_TRUE(st.lookup("x") != nullptr);
    ASSERT_TRUE(st.lookup("y") == nullptr);
}

TEST(symbol_table_scopes) {
    SymbolTable st;
    
    st.define("global_var", SymbolKind::Variable, i32_type());
    
    st.enter_scope(Scope::Kind::Function);
    st.define("local_var", SymbolKind::Variable, i64_type());
    
    ASSERT_TRUE(st.lookup("global_var") != nullptr);
    ASSERT_TRUE(st.lookup("local_var") != nullptr);
    
    st.exit_scope();
    
    ASSERT_TRUE(st.lookup("global_var") != nullptr);
    ASSERT_TRUE(st.lookup("local_var") == nullptr);
}

TEST(symbol_table_shadowing) {
    SymbolTable st;
    
    st.define("x", SymbolKind::Variable, i32_type());
    
    st.enter_scope(Scope::Kind::Block);
    st.define("x", SymbolKind::Variable, string_type());
    
    auto sym = st.lookup("x");
    ASSERT_TRUE(sym->type->kind == TypeKind::String);
    
    st.exit_scope();
    
    sym = st.lookup("x");
    ASSERT_TRUE(sym->type->kind == TypeKind::Int32);
}

TEST(builtin_types) {
    SymbolTable st;
    
    ASSERT_TRUE(st.lookup_type("i32") != nullptr);
    ASSERT_TRUE(st.lookup_type("str") != nullptr);
    ASSERT_TRUE(st.lookup_type("bool") != nullptr);
}

TEST(builtin_functions) {
    SymbolTable st;
    
    ASSERT_TRUE(st.lookup("print") != nullptr);
    ASSERT_TRUE(st.lookup("len") != nullptr);
    ASSERT_TRUE(st.lookup("range") != nullptr);
}

// === Type Checker Tests ===

TEST(check_simple_function) {
    auto program = parse(R"(
fn foo() -> i32:
    return 42
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

TEST(check_function_params) {
    auto program = parse(R"(
fn add(a: i32, b: i32) -> i32:
    return a + b
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

TEST(check_variable_declaration) {
    auto program = parse(R"(
fn test():
    let x = 10
    let y: i32 = 20
    var z = 30
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

TEST(check_if_statement) {
    auto program = parse(R"(
fn test(x: i32) -> i32:
    if x > 0:
        return 1
    else:
        return 0
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

TEST(check_for_loop) {
    auto program = parse(R"(
fn sum() -> i32:
    var total = 0
    for i in 0..10:
        total = total + i
    return total
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

TEST(check_struct) {
    auto program = parse(R"(
struct Point:
    x: f64
    y: f64
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

TEST(check_function_call) {
    auto program = parse(R"(
fn greet(name: str):
    print(name)

fn main():
    greet("World")
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

TEST(error_undefined_variable) {
    auto program = parse(R"(
fn test():
    let x = undefined_var
)");
    
    TypeChecker checker;
    checker.check(program);
    EXPECT_ERROR(checker);
}

TEST(error_type_mismatch) {
    auto program = parse(R"(
fn test():
    let x: i32 = "not an int"
)");
    
    TypeChecker checker;
    checker.check(program);
    EXPECT_ERROR(checker);
}

TEST(error_immutable_assign) {
    auto program = parse(R"(
fn test():
    let x = 10
    x = 20
)");
    
    TypeChecker checker;
    checker.check(program);
    EXPECT_ERROR(checker);
}

TEST(error_break_outside_loop) {
    auto program = parse(R"(
fn test():
    break
)");
    
    TypeChecker checker;
    checker.check(program);
    EXPECT_ERROR(checker);
}

TEST(check_list_literal) {
    auto program = parse(R"(
fn test():
    let nums = [1, 2, 3]
    let strs = ["a", "b", "c"]
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

TEST(check_dict_literal) {
    auto program = parse(R"(
fn test():
    let scores = {"alice": 100, "bob": 95}
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

TEST(check_binary_ops) {
    auto program = parse(R"(
fn math(a: i32, b: i32) -> bool:
    let sum = a + b
    let diff = a - b
    let prod = a * b
    let eq = a == b
    return eq
)");
    
    TypeChecker checker;
    checker.check(program);
    ASSERT_NO_ERRORS(checker);
}

int main() {
    std::cout << "=== Axiom Semantic Tests ===\n\n";
    
    // Type system
    RUN_TEST(primitive_types);
    RUN_TEST(type_equality);
    RUN_TEST(type_to_string);
    RUN_TEST(type_assignability);
    
    // Symbol table
    RUN_TEST(symbol_table_basic);
    RUN_TEST(symbol_table_scopes);
    RUN_TEST(symbol_table_shadowing);
    RUN_TEST(builtin_types);
    RUN_TEST(builtin_functions);
    
    // Type checker - basic tests
    RUN_TEST(check_simple_function);
    RUN_TEST(check_function_params);
    RUN_TEST(check_variable_declaration);
    RUN_TEST(check_struct);
    RUN_TEST(error_undefined_variable);
    RUN_TEST(error_break_outside_loop);
    RUN_TEST(check_list_literal);
    RUN_TEST(check_binary_ops);
    
    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}

