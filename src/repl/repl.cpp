/**
 * @file repl.cpp
 * @brief Interactive REPL implementation for Axiom
 */

#include "repl.hpp"
#include <iomanip>
#include <algorithm>

namespace axiom {
namespace repl {

// === REPL Implementation ===

Repl::Repl(const ReplConfig& config) : config_(config) {
    codegen::initialize_llvm();
}

void Repl::run() {
    print_welcome();
    
    std::string input;
    while (true) {
        std::cout << config_.prompt;
        
        if (!std::getline(std::cin, input)) {
            std::cout << "\nGoodbye!\n";
            break;
        }
        
        // Handle empty input
        if (input.empty()) continue;
        
        // Handle REPL commands
        if (input[0] == ':') {
            if (!handle_command(input)) break;
            continue;
        }
        
        // Handle multiline input
        if (config_.multiline && needs_more_input(input)) {
            input = read_multiline();
        }
        
        // Execute the code
        execute(input);
        line_number_++;
    }
}

bool Repl::execute(const std::string& code) {
    try {
        // Show tokens if requested
        if (config_.show_tokens) {
            show_tokens(code);
        }
        
        // Parse
        Lexer lexer(code);
        Parser parser(lexer);
        auto program = parser.parse();
        
        // Check for parse errors
        if (parser.has_errors()) {
            for (const auto& err : parser.errors()) {
                print_error(err.message, err.location.line, err.location.column);
            }
            return false;
        }
        
        // Show AST if requested
        if (config_.show_ast) {
            show_ast(program);
        }
        
        // Type check
        type_checker_.check(program);
        
        if (type_checker_.has_errors()) {
            for (const auto& err : type_checker_.errors()) {
                print_error(err.message, err.location.line, err.location.column);
            }
            return false;
        }
        
        // Generate IR if requested
        if (config_.show_ir) {
            codegen::CodeGenerator codegen("repl");
            if (codegen.generate(program, type_checker_)) {
                codegen.dump_ir();
            }
        }
        
        // Show inferred types for expressions
        if (config_.show_types && !program.declarations.empty()) {
            // For now, just indicate success
            std::cout << "✓ OK\n";
        }
        
        return true;
        
    } catch (const std::exception& e) {
        print_error(e.what());
        return false;
    }
}

bool Repl::needs_more_input(const std::string& code) const {
    // Check for incomplete blocks (ends with :)
    std::string trimmed = code;
    while (!trimmed.empty() && std::isspace(trimmed.back())) {
        trimmed.pop_back();
    }
    
    if (trimmed.empty()) return false;
    
    // Ends with colon - likely starts a block
    if (trimmed.back() == ':') return true;
    
    // Check for unbalanced brackets
    int parens = 0, brackets = 0, braces = 0;
    for (char c : code) {
        switch (c) {
            case '(': parens++; break;
            case ')': parens--; break;
            case '[': brackets++; break;
            case ']': brackets--; break;
            case '{': braces++; break;
            case '}': braces--; break;
        }
    }
    
    return parens > 0 || brackets > 0 || braces > 0;
}

std::string Repl::read_multiline() {
    std::string result;
    std::string line;
    int indent_level = 1;
    
    while (indent_level > 0) {
        std::cout << config_.continuation;
        
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        // Empty line ends the block
        if (line.empty()) {
            indent_level--;
            continue;
        }
        
        // Check indent
        size_t spaces = 0;
        while (spaces < line.size() && line[spaces] == ' ') {
            spaces++;
        }
        
        result += "\n" + line;
        
        // Ends with colon - nested block
        std::string trimmed = line;
        while (!trimmed.empty() && std::isspace(trimmed.back())) {
            trimmed.pop_back();
        }
        if (!trimmed.empty() && trimmed.back() == ':') {
            indent_level++;
        }
    }
    
    return result;
}

bool Repl::handle_command(const std::string& input) {
    std::string cmd = input.substr(1);
    
    // Remove leading/trailing whitespace
    while (!cmd.empty() && std::isspace(cmd.front())) cmd.erase(0, 1);
    while (!cmd.empty() && std::isspace(cmd.back())) cmd.pop_back();
    
    if (cmd == "quit" || cmd == "q" || cmd == "exit") {
        std::cout << "Goodbye!\n";
        return false;
    }
    
    if (cmd == "help" || cmd == "h" || cmd == "?") {
        print_help();
        return true;
    }
    
    if (cmd == "tokens" || cmd == "t") {
        config_.show_tokens = !config_.show_tokens;
        std::cout << "Token display: " << (config_.show_tokens ? "ON" : "OFF") << "\n";
        return true;
    }
    
    if (cmd == "ast" || cmd == "a") {
        config_.show_ast = !config_.show_ast;
        std::cout << "AST display: " << (config_.show_ast ? "ON" : "OFF") << "\n";
        return true;
    }
    
    if (cmd == "ir" || cmd == "i") {
        config_.show_ir = !config_.show_ir;
        std::cout << "IR display: " << (config_.show_ir ? "ON" : "OFF") << "\n";
        return true;
    }
    
    if (cmd == "clear" || cmd == "c") {
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif
        return true;
    }
    
    if (cmd == "reset" || cmd == "r") {
        type_checker_ = semantic::TypeChecker();
        line_number_ = 1;
        std::cout << "Session reset.\n";
        return true;
    }
    
    std::cout << "Unknown command: :" << cmd << "\n";
    std::cout << "Type :help for available commands.\n";
    return true;
}

void Repl::print_welcome() {
    std::cout << R"(
   _____        _                 
  /  _  \__  __(_) ____   _____   
 /  /_\  \ \/ /| |/  _ \ /     \  
/    |    \  / | (  <_> )  Y Y  \ 
\____|__  /\_/  |_|\____/|__|_|  / 
        \/                     \/  

)" << "Axiom Programming Language v0.1.0\n"
   << "Type :help for commands, :quit to exit.\n\n";
}

void Repl::print_help() {
    std::cout << R"(
REPL Commands:
  :help, :h, :?     Show this help
  :quit, :q, :exit  Exit the REPL
  :tokens, :t       Toggle token display
  :ast, :a          Toggle AST display
  :ir, :i           Toggle LLVM IR display
  :clear, :c        Clear the screen
  :reset, :r        Reset the session

Examples:
  >>> fn greet(name: str) -> str:
  ...     return "Hello, " + name
  ...
  >>> let x = 42
  >>> print(greet("World"))

)" << std::endl;
}

void Repl::print_error(const std::string& msg, int line, int col) {
    std::cout << "\033[31m" << "error";
    if (line >= 0) {
        std::cout << " (line " << line;
        if (col >= 0) std::cout << ", col " << col;
        std::cout << ")";
    }
    std::cout << ": \033[0m" << msg << "\n";
}

void Repl::show_tokens(const std::string& code) {
    std::cout << "\033[36m--- Tokens ---\033[0m\n";
    Lexer lexer(code);
    for (const auto& token : lexer.tokenize_all()) {
        std::cout << "  " << token << "\n";
    }
    std::cout << "\n";
}

void Repl::show_ast(ast::Program& program) {
    std::cout << "\033[36m--- AST ---\033[0m\n";
    std::cout << "  " << program.declarations.size() << " declaration(s)\n";
    for (size_t i = 0; i < program.declarations.size(); ++i) {
        auto* decl = program.declarations[i].get();
        if (auto* fn = dynamic_cast<ast::FnDecl*>(decl)) {
            std::cout << "  [" << i << "] FnDecl: " << fn->name << "\n";
        } else if (auto* st = dynamic_cast<ast::StructDecl*>(decl)) {
            std::cout << "  [" << i << "] StructDecl: " << st->name << "\n";
        } else {
            std::cout << "  [" << i << "] Declaration\n";
        }
    }
    std::cout << "\n";
}

// === Error Reporter Implementation ===

void ErrorReporter::report(const std::string& filename,
                           const std::string& source,
                           int line, int column,
                           const std::string& message,
                           const std::string& error_type) {
    error_count_++;
    
    std::cerr << BOLD << RED << error_type << RESET << BOLD << ": " << message << RESET << "\n";
    std::cerr << CYAN << "  --> " << RESET << filename << ":" << line << ":" << column << "\n";
    
    std::string source_line = get_line(source, line);
    if (!source_line.empty()) {
        std::cerr << "   " << CYAN << "|" << RESET << "\n";
        std::cerr << " " << std::setw(3) << line << CYAN << " | " << RESET << source_line << "\n";
        std::cerr << "   " << CYAN << "|" << RESET << make_caret(column) << "\n";
    }
}

void ErrorReporter::note(const std::string& message) {
    std::cerr << CYAN << "note" << RESET << ": " << message << "\n";
}

void ErrorReporter::warning(const std::string& filename,
                            const std::string& source,
                            int line, int column,
                            const std::string& message) {
    warning_count_++;
    
    std::cerr << BOLD << YELLOW << "warning" << RESET << BOLD << ": " << message << RESET << "\n";
    std::cerr << CYAN << "  --> " << RESET << filename << ":" << line << ":" << column << "\n";
    
    std::string source_line = get_line(source, line);
    if (!source_line.empty()) {
        std::cerr << "   " << CYAN << "|" << RESET << "\n";
        std::cerr << " " << std::setw(3) << line << CYAN << " | " << RESET << source_line << "\n";
        std::cerr << "   " << CYAN << "|" << RESET << make_caret(column) << "\n";
    }
}

std::string ErrorReporter::get_line(const std::string& source, int line_num) {
    std::istringstream ss(source);
    std::string line;
    int current = 1;
    
    while (std::getline(ss, line)) {
        if (current == line_num) {
            return line;
        }
        current++;
    }
    
    return "";
}

std::string ErrorReporter::make_caret(int column, int length) {
    std::string result(column + 3, ' ');
    result += RED;
    result += std::string(length, '^');
    result += RESET;
    return result;
}

} // namespace repl
} // namespace axiom
