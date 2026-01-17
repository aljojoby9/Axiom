/**
 * @file parser_test.cpp
 * @brief Unit tests for the Axiom parser
 */

#include <iostream>
#include <cassert>
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"

using namespace axiom;

// Test helper macros
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

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "\nAssertion failed: " << #a << " == " << #b << "\n"; \
        std::exit(1); \
    } \
} while(0)

#define ASSERT_NO_ERRORS(parser) do { \
    if (parser.has_errors()) { \
        std::cerr << "\nParser errors:\n"; \
        for (const auto& err : parser.errors()) { \
            std::cerr << "  " << err.location.line << ":" << err.location.column \
                      << ": " << err.message << "\n"; \
        } \
        std::exit(1); \
    } \
} while(0)

// === Test Cases ===

TEST(empty_program) {
    Lexer lexer("");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    ASSERT_TRUE(program.declarations.empty());
}

TEST(simple_function) {
    Lexer lexer(R"(
fn main():
    return 0
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    ASSERT_EQ(program.declarations.size(), 1u);
    
    auto* fn = dynamic_cast<ast::FnDecl*>(program.declarations[0].get());
    ASSERT_TRUE(fn != nullptr);
    ASSERT_EQ(fn->name, "main");
}

TEST(function_with_params) {
    Lexer lexer(R"(
fn add(a: i32, b: i32) -> i32:
    return a + b
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    ASSERT_EQ(program.declarations.size(), 1u);
    
    auto* fn = dynamic_cast<ast::FnDecl*>(program.declarations[0].get());
    ASSERT_TRUE(fn != nullptr);
    ASSERT_EQ(fn->name, "add");
    ASSERT_EQ(fn->params.size(), 2u);
    ASSERT_EQ(fn->params[0].name, "a");
    ASSERT_EQ(fn->params[1].name, "b");
    ASSERT_TRUE(fn->return_type != nullptr);
}

TEST(variable_declaration) {
    Lexer lexer(R"(
fn test():
    let x = 10
    var y: i32 = 20
    const PI = 3.14
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(if_statement) {
    Lexer lexer(R"(
fn test(x: i32):
    if x > 0:
        return 1
    elif x < 0:
        return -1
    else:
        return 0
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(while_loop) {
    Lexer lexer(R"(
fn countdown(n: i32):
    while n > 0:
        n = n - 1
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(for_loop) {
    Lexer lexer(R"(
fn sum_range(n: i32) -> i32:
    var total = 0
    for i in 0..n:
        total = total + i
    return total
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(struct_declaration) {
    Lexer lexer(R"(
struct Point:
    x: f64
    y: f64
    
    fn distance(self, other: Point) -> f64:
        return 0.0
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    ASSERT_EQ(program.declarations.size(), 1u);
    
    auto* st = dynamic_cast<ast::StructDecl*>(program.declarations[0].get());
    ASSERT_TRUE(st != nullptr);
    ASSERT_EQ(st->name, "Point");
    ASSERT_EQ(st->fields.size(), 2u);
}

TEST(class_declaration) {
    Lexer lexer(R"(
class Animal:
    name: str
    
    fn speak(self) -> str:
        return ""
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    
    auto* cls = dynamic_cast<ast::ClassDecl*>(program.declarations[0].get());
    ASSERT_TRUE(cls != nullptr);
    ASSERT_EQ(cls->name, "Animal");
}

TEST(trait_declaration) {
    Lexer lexer(R"(
trait Printable:
    fn to_string(self) -> str:
        return ""
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    
    auto* trait = dynamic_cast<ast::TraitDecl*>(program.declarations[0].get());
    ASSERT_TRUE(trait != nullptr);
    ASSERT_EQ(trait->name, "Printable");
}

TEST(impl_block) {
    Lexer lexer(R"(
impl Point:
    fn new(x: f64, y: f64) -> Point:
        return Point(x, y)
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    
    auto* impl = dynamic_cast<ast::ImplDecl*>(program.declarations[0].get());
    ASSERT_TRUE(impl != nullptr);
    ASSERT_EQ(impl->type_name, "Point");
}

TEST(binary_expressions) {
    Lexer lexer(R"(
fn math():
    let a = 1 + 2 * 3
    let b = 4 - 5 / 6
    let c = 2 ** 10
    let d = a and b or c
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(comparison_expressions) {
    Lexer lexer(R"(
fn compare(a: i32, b: i32):
    let eq = a == b
    let ne = a != b
    let lt = a < b
    let le = a <= b
    let gt = a > b
    let ge = a >= b
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(function_calls) {
    Lexer lexer(R"(
fn caller():
    let x = foo()
    let y = bar(1, 2, 3)
    let z = obj.method()
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(list_literal) {
    Lexer lexer(R"(
fn lists():
    let empty = []
    let nums = [1, 2, 3]
    let mixed = [1, "two", 3.0]
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(list_comprehension) {
    Lexer lexer(R"(
fn comp():
    let squares = [x * x for x in 0..10]
    let evens = [x for x in 0..20 if x % 2 == 0]
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(dict_literal) {
    Lexer lexer(R"(
fn dicts():
    let empty = {}
    let scores = {"alice": 100, "bob": 95}
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(lambda_expression) {
    Lexer lexer(R"(
fn lambdas():
    let square = |x: i32| -> i32 { x * x }
    let add = |a, b| a + b
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(import_statement) {
    Lexer lexer(R"(
import std.collections
from std.math import sin, cos
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    ASSERT_EQ(program.declarations.size(), 2u);
}

TEST(type_annotations) {
    Lexer lexer(R"(
fn types(a: i32, b: List[str], c: Dict[str, i32]) -> Result[i32, str]:
    return Ok(0)
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
}

TEST(async_function) {
    Lexer lexer(R"(
async fn fetch(url: str) -> str:
    let response = await http.get(url)
    return response.body
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    
    auto* fn = dynamic_cast<ast::FnDecl*>(program.declarations[0].get());
    ASSERT_TRUE(fn != nullptr);
    ASSERT_TRUE(fn->is_async);
}

TEST(enum_declaration) {
    Lexer lexer(R"(
enum Color:
    Red
    Green
    Blue
    RGB(i32, i32, i32)
)");
    Parser parser(lexer);
    auto program = parser.parse();
    ASSERT_NO_ERRORS(parser);
    
    auto* en = dynamic_cast<ast::EnumDecl*>(program.declarations[0].get());
    ASSERT_TRUE(en != nullptr);
    ASSERT_EQ(en->name, "Color");
    ASSERT_EQ(en->variants.size(), 4u);
}

// === Main ===

int main() {
    std::cout << "=== Axiom Parser Tests ===\n\n";
    
    RUN_TEST(empty_program);
    RUN_TEST(simple_function);
    RUN_TEST(function_with_params);
    RUN_TEST(variable_declaration);
    RUN_TEST(if_statement);
    RUN_TEST(while_loop);
    RUN_TEST(for_loop);
    RUN_TEST(struct_declaration);
    RUN_TEST(class_declaration);
    RUN_TEST(trait_declaration);
    RUN_TEST(impl_block);
    RUN_TEST(binary_expressions);
    RUN_TEST(comparison_expressions);
    RUN_TEST(function_calls);
    RUN_TEST(list_literal);
    RUN_TEST(list_comprehension);
    RUN_TEST(dict_literal);
    RUN_TEST(lambda_expression);
    RUN_TEST(import_statement);
    RUN_TEST(type_annotations);
    RUN_TEST(async_function);
    RUN_TEST(enum_declaration);
    
    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
