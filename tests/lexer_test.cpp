/**
 * @file lexer_test.cpp
 * @brief Unit tests for the Axiom lexer
 */

#include <iostream>
#include <cassert>
#include <vector>
#include "lexer/lexer.hpp"

using namespace axiom;

// Test helper macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "... "; \
    test_##name(); \
    std::cout << "PASSED\n"; \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::cerr << "\nAssertion failed: " << #a << " == " << #b << "\n"; \
        std::cerr << "  Got: " << (a) << "\n"; \
        std::exit(1); \
    } \
} while(0)

#define ASSERT_TOKEN(tok, expected_type) do { \
    if ((tok).type != (expected_type)) { \
        std::cerr << "\nToken type mismatch\n"; \
        std::cerr << "  Expected: " << token_type_name(expected_type) << "\n"; \
        std::cerr << "  Got: " << token_type_name((tok).type) << "\n"; \
        std::exit(1); \
    } \
} while(0)

// === Test Cases ===

TEST(empty_input) {
    Lexer lexer("");
    auto tokens = lexer.tokenize_all();
    ASSERT_EQ(tokens.size(), 1u);
    ASSERT_TOKEN(tokens[0], TokenType::EOF_TOKEN);
}

TEST(single_integer) {
    Lexer lexer("42");
    auto tokens = lexer.tokenize_all();
    ASSERT_EQ(tokens.size(), 2u);
    ASSERT_TOKEN(tokens[0], TokenType::INTEGER);
    ASSERT_EQ(tokens[0].int_value, 42);
}

TEST(float_literal) {
    Lexer lexer("3.14159");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::FLOAT);
    // Allow small floating point error
    assert(std::abs(tokens[0].float_value - 3.14159) < 0.00001);
}

TEST(hex_literal) {
    Lexer lexer("0xFF");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::INTEGER);
    ASSERT_EQ(tokens[0].int_value, 255);
}

TEST(binary_literal) {
    Lexer lexer("0b1010");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::INTEGER);
    ASSERT_EQ(tokens[0].int_value, 10);
}

TEST(string_literal) {
    Lexer lexer("\"hello world\"");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::STRING);
}

TEST(string_escape_sequences) {
    Lexer lexer("\"line1\\nline2\\ttab\"");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::STRING);
}

TEST(keywords) {
    Lexer lexer("fn let var if else while for return");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::FN);
    ASSERT_TOKEN(tokens[1], TokenType::LET);
    ASSERT_TOKEN(tokens[2], TokenType::VAR);
    ASSERT_TOKEN(tokens[3], TokenType::IF);
    ASSERT_TOKEN(tokens[4], TokenType::ELSE);
    ASSERT_TOKEN(tokens[5], TokenType::WHILE);
    ASSERT_TOKEN(tokens[6], TokenType::FOR);
    ASSERT_TOKEN(tokens[7], TokenType::RETURN);
}

TEST(identifiers) {
    Lexer lexer("foo bar_baz _private CamelCase");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::IDENTIFIER);
    ASSERT_TOKEN(tokens[1], TokenType::IDENTIFIER);
    ASSERT_TOKEN(tokens[2], TokenType::IDENTIFIER);
    ASSERT_TOKEN(tokens[3], TokenType::IDENTIFIER);
}

TEST(operators) {
    Lexer lexer("+ - * / % ** = == != < <= > >= -> =>");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::PLUS);
    ASSERT_TOKEN(tokens[1], TokenType::MINUS);
    ASSERT_TOKEN(tokens[2], TokenType::STAR);
    ASSERT_TOKEN(tokens[3], TokenType::SLASH);
    ASSERT_TOKEN(tokens[4], TokenType::PERCENT);
    ASSERT_TOKEN(tokens[5], TokenType::POWER);
    ASSERT_TOKEN(tokens[6], TokenType::ASSIGN);
    ASSERT_TOKEN(tokens[7], TokenType::EQ);
    ASSERT_TOKEN(tokens[8], TokenType::NE);
    ASSERT_TOKEN(tokens[9], TokenType::LT);
    ASSERT_TOKEN(tokens[10], TokenType::LE);
    ASSERT_TOKEN(tokens[11], TokenType::GT);
    ASSERT_TOKEN(tokens[12], TokenType::GE);
    ASSERT_TOKEN(tokens[13], TokenType::ARROW);
    ASSERT_TOKEN(tokens[14], TokenType::FAT_ARROW);
}

TEST(delimiters) {
    Lexer lexer("( ) [ ] { } , : ; . ::");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::LPAREN);
    ASSERT_TOKEN(tokens[1], TokenType::RPAREN);
    ASSERT_TOKEN(tokens[2], TokenType::LBRACKET);
    ASSERT_TOKEN(tokens[3], TokenType::RBRACKET);
    ASSERT_TOKEN(tokens[4], TokenType::LBRACE);
    ASSERT_TOKEN(tokens[5], TokenType::RBRACE);
    ASSERT_TOKEN(tokens[6], TokenType::COMMA);
    ASSERT_TOKEN(tokens[7], TokenType::COLON);
    ASSERT_TOKEN(tokens[8], TokenType::SEMICOLON);
    ASSERT_TOKEN(tokens[9], TokenType::DOT);
    ASSERT_TOKEN(tokens[10], TokenType::DOUBLE_COLON);
}

TEST(comments) {
    Lexer lexer("x # this is a comment\ny");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::IDENTIFIER);
    ASSERT_TOKEN(tokens[1], TokenType::NEWLINE);
    ASSERT_TOKEN(tokens[2], TokenType::IDENTIFIER);
}

TEST(simple_function) {
    Lexer lexer(R"(
fn add(a: i32, b: i32) -> i32:
    return a + b
)");
    auto tokens = lexer.tokenize_all();
    
    // Check key tokens are present
    bool found_fn = false, found_arrow = false, found_return = false;
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::FN) found_fn = true;
        if (tok.type == TokenType::ARROW) found_arrow = true;
        if (tok.type == TokenType::RETURN) found_return = true;
    }
    assert(found_fn && found_arrow && found_return);
}

TEST(logical_operators) {
    Lexer lexer("and or not");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::AND);
    ASSERT_TOKEN(tokens[1], TokenType::OR);
    ASSERT_TOKEN(tokens[2], TokenType::NOT);
}

TEST(boolean_literals) {
    Lexer lexer("true false None");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::TRUE);
    ASSERT_TOKEN(tokens[1], TokenType::FALSE);
    ASSERT_TOKEN(tokens[2], TokenType::NONE);
}

TEST(range_operators) {
    Lexer lexer("0..10 ...");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::INTEGER);
    ASSERT_TOKEN(tokens[1], TokenType::DOUBLE_DOT);
    ASSERT_TOKEN(tokens[2], TokenType::INTEGER);
    ASSERT_TOKEN(tokens[3], TokenType::TRIPLE_DOT);
}

TEST(scientific_notation) {
    Lexer lexer("1e10 2.5e-3 1E+5");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::FLOAT);
    ASSERT_TOKEN(tokens[1], TokenType::FLOAT);
    ASSERT_TOKEN(tokens[2], TokenType::FLOAT);
}

TEST(compound_assignment) {
    Lexer lexer("+= -= *= /= %=");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::PLUS_ASSIGN);
    ASSERT_TOKEN(tokens[1], TokenType::MINUS_ASSIGN);
    ASSERT_TOKEN(tokens[2], TokenType::STAR_ASSIGN);
    ASSERT_TOKEN(tokens[3], TokenType::SLASH_ASSIGN);
    ASSERT_TOKEN(tokens[4], TokenType::PERCENT_ASSIGN);
}

TEST(bitwise_operators) {
    Lexer lexer("& | ^ ~ << >>");
    auto tokens = lexer.tokenize_all();
    ASSERT_TOKEN(tokens[0], TokenType::AMPERSAND);
    ASSERT_TOKEN(tokens[1], TokenType::PIPE);
    ASSERT_TOKEN(tokens[2], TokenType::CARET);
    ASSERT_TOKEN(tokens[3], TokenType::TILDE);
    ASSERT_TOKEN(tokens[4], TokenType::SHL);
    ASSERT_TOKEN(tokens[5], TokenType::SHR);
}

// === Main ===

int main() {
    std::cout << "=== Axiom Lexer Tests ===\n\n";
    
    RUN_TEST(empty_input);
    RUN_TEST(single_integer);
    RUN_TEST(float_literal);
    RUN_TEST(hex_literal);
    RUN_TEST(binary_literal);
    RUN_TEST(string_literal);
    RUN_TEST(string_escape_sequences);
    RUN_TEST(keywords);
    RUN_TEST(identifiers);
    RUN_TEST(operators);
    RUN_TEST(delimiters);
    RUN_TEST(comments);
    RUN_TEST(simple_function);
    RUN_TEST(logical_operators);
    RUN_TEST(boolean_literals);
    RUN_TEST(range_operators);
    RUN_TEST(scientific_notation);
    RUN_TEST(compound_assignment);
    RUN_TEST(bitwise_operators);
    
    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
