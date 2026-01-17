#pragma once

/**
 * @file token.hpp
 * @brief Token definitions for the Axiom lexer
 * 
 * This file defines all token types recognized by the Axiom language,
 * including keywords, operators, literals, and structural tokens.
 */

#include <string>
#include <string_view>
#include <cstdint>
#include <ostream>

namespace axiom {

/**
 * @brief Source location information for error reporting
 */
struct SourceLocation {
    std::string_view filename;
    size_t line;
    size_t column;
    size_t offset;  // Byte offset in source
    
    SourceLocation() : filename(""), line(1), column(1), offset(0) {}
    SourceLocation(std::string_view file, size_t l, size_t c, size_t o)
        : filename(file), line(l), column(c), offset(o) {}
};

/**
 * @brief All token types in the Axiom language
 */
enum class TokenType {
    // === Literals ===
    INTEGER,        // 42, 0xFF, 0b1010
    FLOAT,          // 3.14, 1e-10
    STRING,         // "hello", 'world'
    CHAR,           // 'a' (single character)
    TRUE,           // true
    FALSE,          // false
    NONE,           // None
    
    // === Identifiers ===
    IDENTIFIER,     // variable_name, ClassName
    
    // === Keywords - Declarations ===
    FN,             // fn
    LET,            // let (immutable)
    VAR,            // var (mutable)
    CONST,          // const (compile-time)
    STRUCT,         // struct
    CLASS,          // class
    TRAIT,          // trait
    IMPL,           // impl
    ENUM,           // enum
    TYPE,           // type (type alias)
    
    // === Keywords - Control Flow ===
    IF,             // if
    ELSE,           // else
    ELIF,           // elif
    MATCH,          // match
    CASE,           // case
    WHILE,          // while
    FOR,            // for
    IN,             // in
    BREAK,          // break
    CONTINUE,       // continue
    RETURN,         // return
    YIELD,          // yield
    
    // === Keywords - Async ===
    ASYNC,          // async
    AWAIT,          // await
    SPAWN,          // spawn
    
    // === Keywords - Other ===
    IMPORT,         // import
    FROM,           // from
    AS,             // as
    PUB,            // pub (public)
    MUT,            // mut (mutable reference)
    SELF,           // self
    SELF_TYPE,      // Self (type)
    SUPER,          // super
    
    // === Arithmetic Operators ===
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /
    PERCENT,        // %
    POWER,          // **
    
    // === Comparison Operators ===
    EQ,             // ==
    NE,             // !=
    LT,             // <
    LE,             // <=
    GT,             // >
    GE,             // >=
    
    // === Logical Operators ===
    AND,            // and
    OR,             // or
    NOT,            // not
    
    // === Bitwise Operators ===
    AMPERSAND,      // &
    PIPE,           // |
    CARET,          // ^
    TILDE,          // ~
    SHL,            // <<
    SHR,            // >>
    
    // === Assignment Operators ===
    ASSIGN,         // =
    PLUS_ASSIGN,    // +=
    MINUS_ASSIGN,   // -=
    STAR_ASSIGN,    // *=
    SLASH_ASSIGN,   // /=
    PERCENT_ASSIGN, // %=
    
    // === Special Operators ===
    ARROW,          // ->
    FAT_ARROW,      // =>
    QUESTION,       // ?
    AT,             // @
    DOUBLE_DOT,     // ..
    TRIPLE_DOT,     // ...
    
    // === Delimiters ===
    LPAREN,         // (
    RPAREN,         // )
    LBRACKET,       // [
    RBRACKET,       // ]
    LBRACE,         // {
    RBRACE,         // }
    COMMA,          // ,
    COLON,          // :
    SEMICOLON,      // ;
    DOT,            // .
    DOUBLE_COLON,   // ::
    
    // === Whitespace/Structure ===
    NEWLINE,        // \n (significant in Python-style mode)
    INDENT,         // Increase in indentation
    DEDENT,         // Decrease in indentation
    
    // === Special ===
    EOF_TOKEN,      // End of file
    ERROR,          // Lexer error
    
    // Count (for iteration)
    _COUNT
};

/**
 * @brief Get string representation of a token type
 */
std::string_view token_type_name(TokenType type);

/**
 * @brief A token produced by the lexer
 */
struct Token {
    TokenType type;
    std::string lexeme;         // The actual text
    SourceLocation location;
    
    // Literal values (populated for literal tokens)
    union {
        int64_t int_value;
        double float_value;
    };
    
    Token() : type(TokenType::ERROR), lexeme(""), int_value(0) {}
    
    Token(TokenType t, std::string_view lex, SourceLocation loc)
        : type(t), lexeme(lex), location(loc), int_value(0) {}
    
    // Convenience constructors
    static Token make_int(int64_t value, std::string_view lex, SourceLocation loc) {
        Token t(TokenType::INTEGER, lex, loc);
        t.int_value = value;
        return t;
    }
    
    static Token make_float(double value, std::string_view lex, SourceLocation loc) {
        Token t(TokenType::FLOAT, lex, loc);
        t.float_value = value;
        return t;
    }
    
    static Token make_string(std::string_view value, SourceLocation loc) {
        return Token(TokenType::STRING, value, loc);
    }
    
    static Token make_error(std::string_view message, SourceLocation loc) {
        return Token(TokenType::ERROR, message, loc);
    }
    
    static Token make_eof(SourceLocation loc) {
        return Token(TokenType::EOF_TOKEN, "", loc);
    }
    
    bool is_eof() const { return type == TokenType::EOF_TOKEN; }
    bool is_error() const { return type == TokenType::ERROR; }
    bool is_literal() const {
        return type == TokenType::INTEGER || type == TokenType::FLOAT ||
               type == TokenType::STRING || type == TokenType::CHAR ||
               type == TokenType::TRUE || type == TokenType::FALSE ||
               type == TokenType::NONE;
    }
    bool is_keyword() const {
        return type >= TokenType::FN && type <= TokenType::SUPER;
    }
};

/**
 * @brief Output stream operator for tokens (debugging)
 */
std::ostream& operator<<(std::ostream& os, const Token& token);
std::ostream& operator<<(std::ostream& os, TokenType type);

} // namespace axiom
