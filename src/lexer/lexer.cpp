/**
 * @file lexer.cpp
 * @brief Lexer implementation for the Axiom programming language
 */

#include "lexer.hpp"
#include <charconv>
#include <cmath>
#include <sstream>

namespace axiom {

// Keyword lookup table
const std::unordered_map<std::string_view, TokenType> Lexer::KEYWORDS = {
    // Keywords - Declarations
    {"fn", TokenType::FN},
    {"let", TokenType::LET},
    {"var", TokenType::VAR},
    {"const", TokenType::CONST},
    {"struct", TokenType::STRUCT},
    {"class", TokenType::CLASS},
    {"trait", TokenType::TRAIT},
    {"impl", TokenType::IMPL},
    {"enum", TokenType::ENUM},
    {"type", TokenType::TYPE},
    
    // Keywords - Control Flow
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"elif", TokenType::ELIF},
    {"match", TokenType::MATCH},
    {"case", TokenType::CASE},
    {"while", TokenType::WHILE},
    {"for", TokenType::FOR},
    {"in", TokenType::IN},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"return", TokenType::RETURN},
    {"yield", TokenType::YIELD},
    
    // Keywords - Async
    {"async", TokenType::ASYNC},
    {"await", TokenType::AWAIT},
    {"spawn", TokenType::SPAWN},
    
    // Keywords - Other
    {"import", TokenType::IMPORT},
    {"from", TokenType::FROM},
    {"as", TokenType::AS},
    {"pub", TokenType::PUB},
    {"mut", TokenType::MUT},
    {"self", TokenType::SELF},
    {"Self", TokenType::SELF_TYPE},
    {"super", TokenType::SUPER},
    
    // Literals
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"None", TokenType::NONE},
    
    // Logical operators (keyword form)
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT},
};

Lexer::Lexer(std::string_view source, std::string_view filename)
    : source_(source), filename_(filename) {
    // Initialize indentation stack with base level 0
    indent_stack_.push(0);
}

Token Lexer::next_token() {
    // Return cached peeked token if available
    if (has_peeked_) {
        has_peeked_ = false;
        return peeked_token_;
    }
    
    // Handle pending dedents
    if (pending_dedents_ > 0) {
        pending_dedents_--;
        return make_token(TokenType::DEDENT, "");
    }
    
    // Handle indentation at start of line
    if (at_line_start_) {
        handle_indentation();
        if (pending_dedents_ > 0) {
            pending_dedents_--;
            return make_token(TokenType::DEDENT, "");
        }
    }
    
    return scan_token();
}

Token Lexer::peek_token() {
    if (!has_peeked_) {
        peeked_token_ = next_token();
        has_peeked_ = true;
    }
    return peeked_token_;
}

std::vector<Token> Lexer::tokenize_all() {
    std::vector<Token> tokens;
    while (true) {
        Token token = next_token();
        tokens.push_back(token);
        if (token.type == TokenType::EOF_TOKEN) {
            break;
        }
    }
    return tokens;
}

// === Source Navigation ===

char Lexer::peek() const {
    if (is_at_end()) return '\0';
    return source_[current_];
}

char Lexer::peek_next() const {
    if (current_ + 1 >= source_.size()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    char c = source_[current_++];
    if (c == '\n') {
        line_++;
        line_start_ = current_;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (source_[current_] != expected) return false;
    advance();
    return true;
}

bool Lexer::match_sequence(std::string_view seq) {
    if (current_ + seq.size() > source_.size()) return false;
    if (source_.substr(current_, seq.size()) != seq) return false;
    for (size_t i = 0; i < seq.size(); i++) {
        advance();
    }
    return true;
}

// === Token Creation ===

Token Lexer::make_token(TokenType type) {
    std::string_view lexeme = source_.substr(start_, current_ - start_);
    return make_token(type, lexeme);
}

Token Lexer::make_token(TokenType type, std::string_view lexeme) {
    SourceLocation loc(filename_, line_, column_ - lexeme.size(), start_);
    return Token(type, lexeme, loc);
}

Token Lexer::error_token(std::string_view message) {
    SourceLocation loc(filename_, line_, column_, current_);
    Token err = Token::make_error(message, loc);
    errors_.push_back(err);
    return err;
}

// === Main Scanning ===

Token Lexer::scan_token() {
    skip_whitespace_inline();
    
    if (is_at_end()) {
        // Emit remaining dedents at EOF
        if (indent_stack_.size() > 1) {
            indent_stack_.pop();
            return make_token(TokenType::DEDENT, "");
        }
        return Token::make_eof(SourceLocation(filename_, line_, column_, current_));
    }
    
    start_ = current_;
    char c = advance();
    
    // Newline
    if (c == '\n') {
        at_line_start_ = true;
        return make_token(TokenType::NEWLINE);
    }
    
    // Identifiers and keywords
    if (is_identifier_start(c)) {
        return scan_identifier();
    }
    
    // Numbers
    if (is_digit(c)) {
        return scan_number();
    }
    
    // String literals
    if (c == '"' || c == '\'') {
        return scan_string(c);
    }
    
    // F-strings
    if ((c == 'f' || c == 'F') && (peek() == '"' || peek() == '\'')) {
        return scan_fstring();
    }
    
    // Operators and punctuation
    switch (c) {
        // Single-character tokens
        case '(': return make_token(TokenType::LPAREN);
        case ')': return make_token(TokenType::RPAREN);
        case '[': return make_token(TokenType::LBRACKET);
        case ']': return make_token(TokenType::RBRACKET);
        case '{': return make_token(TokenType::LBRACE);
        case '}': return make_token(TokenType::RBRACE);
        case ',': return make_token(TokenType::COMMA);
        case ';': return make_token(TokenType::SEMICOLON);
        case '~': return make_token(TokenType::TILDE);
        case '@': return make_token(TokenType::AT);
        case '?': return make_token(TokenType::QUESTION);
        
        // Potentially multi-character tokens
        case '+':
            if (match('=')) return make_token(TokenType::PLUS_ASSIGN);
            return make_token(TokenType::PLUS);
            
        case '-':
            if (match('>')) return make_token(TokenType::ARROW);
            if (match('=')) return make_token(TokenType::MINUS_ASSIGN);
            return make_token(TokenType::MINUS);
            
        case '*':
            if (match('*')) return make_token(TokenType::POWER);
            if (match('=')) return make_token(TokenType::STAR_ASSIGN);
            return make_token(TokenType::STAR);
            
        case '/':
            if (match('=')) return make_token(TokenType::SLASH_ASSIGN);
            return make_token(TokenType::SLASH);
            
        case '%':
            if (match('=')) return make_token(TokenType::PERCENT_ASSIGN);
            return make_token(TokenType::PERCENT);
            
        case '=':
            if (match('>')) return make_token(TokenType::FAT_ARROW);
            if (match('=')) return make_token(TokenType::EQ);
            return make_token(TokenType::ASSIGN);
            
        case '!':
            if (match('=')) return make_token(TokenType::NE);
            return error_token("Unexpected character '!'");
            
        case '<':
            if (match('<')) return make_token(TokenType::SHL);
            if (match('=')) return make_token(TokenType::LE);
            return make_token(TokenType::LT);
            
        case '>':
            if (match('>')) return make_token(TokenType::SHR);
            if (match('=')) return make_token(TokenType::GE);
            return make_token(TokenType::GT);
            
        case '&':
            return make_token(TokenType::AMPERSAND);
            
        case '|':
            return make_token(TokenType::PIPE);
            
        case '^':
            return make_token(TokenType::CARET);
            
        case ':':
            if (match(':')) return make_token(TokenType::DOUBLE_COLON);
            return make_token(TokenType::COLON);
            
        case '.':
            if (match('.')) {
                if (match('.')) return make_token(TokenType::TRIPLE_DOT);
                return make_token(TokenType::DOUBLE_DOT);
            }
            // Check for float starting with .
            if (is_digit(peek())) {
                return scan_number();
            }
            return make_token(TokenType::DOT);
            
        case '#':
            skip_line_comment();
            return scan_token();  // Get next token after comment
    }
    
    return error_token("Unexpected character");
}

// === Identifier Scanning ===

Token Lexer::scan_identifier() {
    while (is_identifier_char(peek())) {
        advance();
    }
    
    std::string_view text = source_.substr(start_, current_ - start_);
    
    // Check if it's a keyword
    auto it = KEYWORDS.find(text);
    if (it != KEYWORDS.end()) {
        return make_token(it->second);
    }
    
    return make_token(TokenType::IDENTIFIER);
}

// === Number Scanning ===

Token Lexer::scan_number() {
    bool is_float = false;
    
    // Check for hex, binary, octal
    if (source_[start_] == '0' && current_ < source_.size()) {
        char next = peek();
        
        if (next == 'x' || next == 'X') {
            // Hexadecimal
            advance();
            while (is_hex_digit(peek())) advance();
            std::string_view text = source_.substr(start_, current_ - start_);
            
            int64_t value = 0;
            auto [ptr, ec] = std::from_chars(text.data() + 2, text.data() + text.size(), value, 16);
            if (ec != std::errc()) {
                return error_token("Invalid hexadecimal literal");
            }
            
            SourceLocation loc(filename_, line_, column_ - text.size(), start_);
            return Token::make_int(value, text, loc);
        }
        
        if (next == 'b' || next == 'B') {
            // Binary
            advance();
            while (peek() == '0' || peek() == '1') advance();
            std::string_view text = source_.substr(start_, current_ - start_);
            
            int64_t value = 0;
            auto [ptr, ec] = std::from_chars(text.data() + 2, text.data() + text.size(), value, 2);
            if (ec != std::errc()) {
                return error_token("Invalid binary literal");
            }
            
            SourceLocation loc(filename_, line_, column_ - text.size(), start_);
            return Token::make_int(value, text, loc);
        }
        
        if (next == 'o' || next == 'O') {
            // Octal
            advance();
            while (peek() >= '0' && peek() <= '7') advance();
            std::string_view text = source_.substr(start_, current_ - start_);
            
            int64_t value = 0;
            auto [ptr, ec] = std::from_chars(text.data() + 2, text.data() + text.size(), value, 8);
            if (ec != std::errc()) {
                return error_token("Invalid octal literal");
            }
            
            SourceLocation loc(filename_, line_, column_ - text.size(), start_);
            return Token::make_int(value, text, loc);
        }
    }
    
    // Decimal integer or float
    while (is_digit(peek())) advance();
    
    // Fractional part
    if (peek() == '.' && is_digit(peek_next())) {
        is_float = true;
        advance();  // consume '.'
        while (is_digit(peek())) advance();
    }
    
    // Exponent
    if (peek() == 'e' || peek() == 'E') {
        is_float = true;
        advance();  // consume 'e'
        if (peek() == '+' || peek() == '-') advance();
        while (is_digit(peek())) advance();
    }
    
    std::string_view text = source_.substr(start_, current_ - start_);
    SourceLocation loc(filename_, line_, column_ - text.size(), start_);
    
    if (is_float) {
        // Parse as double
        double value = 0.0;
        auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec != std::errc()) {
            return error_token("Invalid floating-point literal");
        }
        return Token::make_float(value, text, loc);
    } else {
        // Parse as int64
        int64_t value = 0;
        auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec != std::errc()) {
            return error_token("Invalid integer literal");
        }
        return Token::make_int(value, text, loc);
    }
}

// === String Scanning ===

Token Lexer::scan_string(char quote) {
    std::string value;
    bool closed = false;
    
    // Check for triple-quoted string
    bool triple = false;
    if (peek() == quote && peek_next() == quote) {
        advance();
        advance();
        triple = true;
    }
    
    while (!is_at_end()) {
        char c = peek();
        
        if (triple) {
            // Triple-quoted: look for closing """
            if (c == quote && peek_next() == quote) {
                advance();
                if (peek() == quote) {
                    advance();
                    advance();
                    closed = true;
                    break;
                }
                value += c;
                continue;
            }
        } else {
            // Single-quoted: end at quote or newline
            if (c == quote) {
                advance();
                closed = true;
                break;
            }
            if (c == '\n') {
                return error_token("Unterminated string literal");
            }
        }
        
        // Handle escape sequences
        if (c == '\\') {
            advance();
            if (is_at_end()) {
                return error_token("Unterminated string literal");
            }
            char escaped = advance();
            switch (escaped) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case 'r':  value += '\r'; break;
                case '\\': value += '\\'; break;
                case '\'': value += '\''; break;
                case '"':  value += '"';  break;
                case '0':  value += '\0'; break;
                default:
                    // Unknown escape, keep as-is
                    value += '\\';
                    value += escaped;
                    break;
            }
        } else {
            value += advance();
        }
    }
    
    if (!closed) {
        return error_token("Unterminated string literal");
    }
    
    SourceLocation loc(filename_, line_, column_ - value.size() - 2, start_);
    return Token::make_string(value, loc);
}

Token Lexer::scan_fstring() {
    // Skip the 'f' prefix
    char quote = advance();
    return scan_string(quote);
}

// === Indentation Handling ===

void Lexer::handle_indentation() {
    at_line_start_ = false;
    
    // Skip blank lines and comments
    while (!is_at_end()) {
        // Skip whitespace at start of line
        size_t indent = 0;
        while (peek() == ' ' || peek() == '\t') {
            if (peek() == ' ') {
                indent++;
            } else {
                indent += 4;  // Tab = 4 spaces
            }
            advance();
        }
        
        // If line is blank or comment, skip it
        if (peek() == '\n') {
            advance();
            continue;
        }
        if (peek() == '#') {
            skip_line_comment();
            if (peek() == '\n') {
                advance();
                continue;
            }
        }
        
        // Non-blank line found, handle indentation
        size_t current_indent = indent_stack_.top();
        
        if (indent > current_indent) {
            // Increased indentation
            indent_stack_.push(indent);
            start_ = current_;
            // Return INDENT token (will be returned by next_token)
            pending_dedents_ = -1;  // Signal to emit INDENT
        } else if (indent < current_indent) {
            // Decreased indentation - may need multiple DEDENT tokens
            while (!indent_stack_.empty() && indent_stack_.top() > indent) {
                indent_stack_.pop();
                pending_dedents_++;
            }
            if (!indent_stack_.empty() && indent_stack_.top() != indent) {
                // Indentation doesn't match any previous level
                errors_.push_back(error_token("Inconsistent indentation"));
            }
        }
        
        break;
    }
    
    // Handle INDENT token
    if (pending_dedents_ == -1) {
        pending_dedents_ = 0;
        start_ = current_;
    }
}

void Lexer::skip_whitespace_inline() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\\' && peek_next() == '\n') {
            // Line continuation
            advance();
            advance();
        } else {
            break;
        }
    }
    start_ = current_;
}

void Lexer::skip_line_comment() {
    while (!is_at_end() && peek() != '\n') {
        advance();
    }
}

void Lexer::skip_block_comment() {
    // Not used currently (Axiom uses # for comments)
}

// === Character Classification ===

bool Lexer::is_hex_digit(char c) const {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool Lexer::is_alpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

} // namespace axiom
