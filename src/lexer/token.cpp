/**
 * @file token.cpp
 * @brief Token implementation for the Axiom lexer
 */

#include "token.hpp"
#include <array>

namespace axiom {

namespace {

// Token type names for debugging and error messages
constexpr std::array<std::string_view, static_cast<size_t>(TokenType::_COUNT)> TOKEN_NAMES = {{
    // Literals
    "INTEGER",
    "FLOAT",
    "STRING",
    "CHAR",
    "TRUE",
    "FALSE",
    "NONE",
    
    // Identifier
    "IDENTIFIER",
    
    // Keywords - Declarations
    "FN",
    "LET",
    "VAR",
    "CONST",
    "STRUCT",
    "CLASS",
    "TRAIT",
    "IMPL",
    "ENUM",
    "TYPE",
    
    // Keywords - Control Flow
    "IF",
    "ELSE",
    "ELIF",
    "MATCH",
    "CASE",
    "WHILE",
    "FOR",
    "IN",
    "BREAK",
    "CONTINUE",
    "RETURN",
    "YIELD",
    
    // Keywords - Async
    "ASYNC",
    "AWAIT",
    "SPAWN",
    
    // Keywords - Other
    "IMPORT",
    "FROM",
    "AS",
    "PUB",
    "MUT",
    "SELF",
    "SELF_TYPE",
    "SUPER",
    
    // Arithmetic Operators
    "PLUS",
    "MINUS",
    "STAR",
    "SLASH",
    "PERCENT",
    "POWER",
    
    // Comparison Operators
    "EQ",
    "NE",
    "LT",
    "LE",
    "GT",
    "GE",
    
    // Logical Operators
    "AND",
    "OR",
    "NOT",
    
    // Bitwise Operators
    "AMPERSAND",
    "PIPE",
    "CARET",
    "TILDE",
    "SHL",
    "SHR",
    
    // Assignment Operators
    "ASSIGN",
    "PLUS_ASSIGN",
    "MINUS_ASSIGN",
    "STAR_ASSIGN",
    "SLASH_ASSIGN",
    "PERCENT_ASSIGN",
    
    // Special Operators
    "ARROW",
    "FAT_ARROW",
    "QUESTION",
    "AT",
    "DOUBLE_DOT",
    "TRIPLE_DOT",
    
    // Delimiters
    "LPAREN",
    "RPAREN",
    "LBRACKET",
    "RBRACKET",
    "LBRACE",
    "RBRACE",
    "COMMA",
    "COLON",
    "SEMICOLON",
    "DOT",
    "DOUBLE_COLON",
    
    // Whitespace/Structure
    "NEWLINE",
    "INDENT",
    "DEDENT",
    
    // Special
    "EOF",
    "ERROR",
}};

} // anonymous namespace

std::string_view token_type_name(TokenType type) {
    auto index = static_cast<size_t>(type);
    if (index < TOKEN_NAMES.size()) {
        return TOKEN_NAMES[index];
    }
    return "UNKNOWN";
}

std::ostream& operator<<(std::ostream& os, TokenType type) {
    os << token_type_name(type);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "Token(" << token.type;
    
    if (!token.lexeme.empty()) {
        os << ", \"" << token.lexeme << "\"";
    }
    
    os << ", " << token.location.line << ":" << token.location.column;
    
    if (token.type == TokenType::INTEGER) {
        os << ", value=" << token.int_value;
    } else if (token.type == TokenType::FLOAT) {
        os << ", value=" << token.float_value;
    }
    
    os << ")";
    return os;
}

} // namespace axiom
