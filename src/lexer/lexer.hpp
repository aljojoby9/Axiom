#pragma once

/**
 * @file lexer.hpp
 * @brief Lexer for the Axiom programming language
 * 
 * The lexer converts source code into a stream of tokens.
 * It handles Python-style significant indentation.
 */

#include "token.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <stack>
#include <unordered_map>

namespace axiom {

/**
 * @brief Lexer for tokenizing Axiom source code
 * 
 * Features:
 * - Python-style significant indentation (INDENT/DEDENT tokens)
 * - All numeric literals (int, float, hex, binary, octal)
 * - String literals with escape sequences
 * - F-strings (parsed as STRING for now)
 * - All operators and keywords
 */
class Lexer {
public:
    /**
     * @brief Construct a lexer for the given source code
     * @param source The source code to tokenize
     * @param filename Optional filename for error messages
     */
    explicit Lexer(std::string_view source, std::string_view filename = "<input>");
    
    /**
     * @brief Get the next token from the source
     * @return The next token, or EOF_TOKEN at end
     */
    Token next_token();
    
    /**
     * @brief Peek at the next token without consuming it
     * @return The next token
     */
    Token peek_token();
    
    /**
     * @brief Check if we've reached the end of input
     */
    bool is_at_end() const { return current_ >= source_.size(); }
    
    /**
     * @brief Get all tokens at once (for debugging)
     */
    std::vector<Token> tokenize_all();
    
    /**
     * @brief Get all errors encountered during lexing
     */
    const std::vector<Token>& errors() const { return errors_; }
    
    /**
     * @brief Check if any errors occurred
     */
    bool has_errors() const { return !errors_.empty(); }

private:
    // Source navigation
    char peek() const;
    char peek_next() const;
    char advance();
    bool match(char expected);
    bool match_sequence(std::string_view seq);
    
    // Token creation helpers
    Token make_token(TokenType type);
    Token make_token(TokenType type, std::string_view lexeme);
    Token error_token(std::string_view message);
    
    // Scanning functions
    Token scan_token();
    Token scan_identifier();
    Token scan_number();
    Token scan_string(char quote);
    Token scan_fstring();
    
    // Indentation handling
    void handle_indentation();
    void skip_whitespace_inline();  // Skip spaces/tabs but not newlines
    void skip_line_comment();
    void skip_block_comment();
    
    // Character classification
    bool is_digit(char c) const { return c >= '0' && c <= '9'; }
    bool is_hex_digit(char c) const;
    bool is_alpha(char c) const;
    bool is_alphanumeric(char c) const { return is_alpha(c) || is_digit(c); }
    bool is_identifier_start(char c) const { return is_alpha(c) || c == '_'; }
    bool is_identifier_char(char c) const { return is_alphanumeric(c) || c == '_'; }
    
    // State
    std::string_view source_;
    std::string_view filename_;
    size_t start_ = 0;          // Start of current token
    size_t current_ = 0;        // Current position
    size_t line_ = 1;           // Current line (1-indexed)
    size_t column_ = 1;         // Current column (1-indexed)
    size_t line_start_ = 0;     // Offset of current line start
    
    // Indentation tracking
    std::stack<size_t> indent_stack_;
    int pending_dedents_ = 0;
    bool at_line_start_ = true;
    
    // Peeked token cache
    bool has_peeked_ = false;
    Token peeked_token_;
    
    // Error collection
    std::vector<Token> errors_;
    
    // Keyword lookup table
    static const std::unordered_map<std::string_view, TokenType> KEYWORDS;
};

} // namespace axiom
