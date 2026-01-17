#pragma once

/**
 * @file repl.hpp
 * @brief Interactive REPL (Read-Eval-Print-Loop) for Axiom
 * 
 * Provides an interactive shell for executing Axiom code.
 */

#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "../semantic/type_checker.hpp"
#include "../codegen/codegen.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

namespace axiom {
namespace repl {

/**
 * @brief REPL configuration
 */
struct ReplConfig {
    bool show_tokens = false;     // Show tokenized output
    bool show_ast = false;        // Show AST structure
    bool show_ir = false;         // Show LLVM IR
    bool show_types = true;       // Show inferred types
    bool multiline = true;        // Support multiline input
    std::string prompt = ">>> ";
    std::string continuation = "... ";
};

/**
 * @brief Interactive REPL for Axiom
 */
class Repl {
public:
    explicit Repl(const ReplConfig& config = ReplConfig{});
    
    /**
     * @brief Start the REPL session
     */
    void run();
    
    /**
     * @brief Execute a single line/block of code
     * @return true if execution succeeded
     */
    bool execute(const std::string& code);
    
    /**
     * @brief Check if input needs more lines (incomplete statement)
     */
    bool needs_more_input(const std::string& code) const;

private:
    ReplConfig config_;
    semantic::TypeChecker type_checker_;
    int line_number_ = 1;
    
    void print_welcome();
    void print_help();
    void print_error(const std::string& msg, int line = -1, int col = -1);
    
    bool handle_command(const std::string& input);
    std::string read_multiline();
    
    void show_tokens(const std::string& code);
    void show_ast(ast::Program& program);
};

/**
 * @brief Pretty error printer with source context
 */
class ErrorReporter {
public:
    ErrorReporter() = default;
    
    /**
     * @brief Report an error with source context
     */
    void report(const std::string& filename,
                const std::string& source,
                int line, int column,
                const std::string& message,
                const std::string& error_type = "error");
    
    /**
     * @brief Report a note/hint
     */
    void note(const std::string& message);
    
    /**
     * @brief Report a warning
     */
    void warning(const std::string& filename,
                 const std::string& source,
                 int line, int column,
                 const std::string& message);
    
    /**
     * @brief Get error count
     */
    int error_count() const { return error_count_; }
    
    /**
     * @brief Get warning count
     */
    int warning_count() const { return warning_count_; }
    
    /**
     * @brief Reset counters
     */
    void reset() { error_count_ = 0; warning_count_ = 0; }

private:
    int error_count_ = 0;
    int warning_count_ = 0;
    
    std::string get_line(const std::string& source, int line_num);
    std::string make_caret(int column, int length = 1);
    
    // ANSI color codes
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* BOLD = "\033[1m";
    static constexpr const char* RESET = "\033[0m";
};

} // namespace repl
} // namespace axiom
