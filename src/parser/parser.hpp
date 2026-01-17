#pragma once

/**
 * @file parser.hpp
 * @brief Recursive descent parser for Axiom
 * 
 * Parses tokens into an Abstract Syntax Tree (AST).
 * Uses Pratt parsing for operator precedence.
 */

#include "ast.hpp"
#include "../lexer/lexer.hpp"
#include <functional>

namespace axiom {

/**
 * @brief Parser error information
 */
struct ParseError {
    std::string message;
    SourceLocation location;
    
    ParseError(std::string msg, SourceLocation loc)
        : message(std::move(msg)), location(loc) {}
};

/**
 * @brief Recursive descent parser for Axiom source code
 */
class Parser {
public:
    /**
     * @brief Construct a parser for the given lexer
     */
    explicit Parser(Lexer& lexer);
    
    /**
     * @brief Parse the entire source into a Program AST
     */
    ast::Program parse();
    
    /**
     * @brief Check if any parse errors occurred
     */
    bool has_errors() const { return !errors_.empty(); }
    
    /**
     * @brief Get all parse errors
     */
    const std::vector<ParseError>& errors() const { return errors_; }

private:
    // Token management
    Token current_token();
    Token peek_token();
    Token advance();
    bool check(TokenType type);
    bool match(TokenType type);
    bool match_any(std::initializer_list<TokenType> types);
    Token expect(TokenType type, const std::string& message);
    
    // Error handling
    void error(const std::string& message);
    void synchronize();  // Panic mode recovery
    
    // === Declaration Parsing ===
    ast::DeclPtr parse_declaration();
    ast::DeclPtr parse_function();
    ast::DeclPtr parse_struct();
    ast::DeclPtr parse_class();
    ast::DeclPtr parse_trait();
    ast::DeclPtr parse_impl();
    ast::DeclPtr parse_enum();
    ast::DeclPtr parse_type_alias();
    ast::DeclPtr parse_import();
    
    // === Statement Parsing ===
    ast::StmtPtr parse_statement();
    ast::StmtPtr parse_if_statement();
    ast::StmtPtr parse_while_statement();
    ast::StmtPtr parse_for_statement();
    ast::StmtPtr parse_match_statement();
    ast::StmtPtr parse_return_statement();
    ast::StmtPtr parse_var_decl_statement();
    ast::BlockPtr parse_block();
    
    // === Expression Parsing (Pratt) ===
    ast::ExprPtr parse_expression();
    ast::ExprPtr parse_expression_with_precedence(int min_precedence);
    ast::ExprPtr parse_prefix();
    ast::ExprPtr parse_infix(ast::ExprPtr left, int precedence);
    ast::ExprPtr parse_postfix(ast::ExprPtr operand);
    
    // Primary expressions
    ast::ExprPtr parse_primary();
    ast::ExprPtr parse_identifier_or_call();
    ast::ExprPtr parse_number();
    ast::ExprPtr parse_string();
    ast::ExprPtr parse_list_or_comprehension();
    ast::ExprPtr parse_dict_or_set();
    ast::ExprPtr parse_tuple_or_grouped();
    ast::ExprPtr parse_lambda();
    
    // === Type Parsing ===
    ast::TypePtr parse_type();
    ast::TypePtr parse_simple_type();
    ast::TypePtr parse_generic_type();
    ast::TypePtr parse_array_type();
    ast::TypePtr parse_function_type();
    
    // === Helpers ===
    ast::FnParam parse_function_param();
    std::vector<ast::FnParam> parse_param_list();
    std::vector<std::string> parse_type_params();
    
    // Operator precedence
    int get_precedence(TokenType type);
    ast::BinaryOp token_to_binary_op(TokenType type);
    ast::UnaryOp token_to_unary_op(TokenType type);
    
    // State
    Lexer& lexer_;
    Token current_;
    Token previous_;
    std::vector<ParseError> errors_;
    bool panic_mode_ = false;
    
    // Indentation-sensitive parsing
    int current_indent_ = 0;
    bool expect_indent_ = false;
};

} // namespace axiom
