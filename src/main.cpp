/**
 * @file main.cpp
 * @brief Axiom compiler entry point
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer/lexer.hpp"

void print_usage(const char* program_name) {
    std::cout << "Axiom Programming Language Compiler v0.1.0\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << program_name << " <file.ax>              Compile and run a file\n";
    std::cout << "  " << program_name << " --lex <file.ax>        Tokenize and show tokens\n";
    std::cout << "  " << program_name << " --help                 Show this help message\n";
    std::cout << "  " << program_name << " --version              Show version information\n";
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: Could not open file '" << path << "'\n";
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int run_lexer(const std::string& source, const std::string& filename) {
    axiom::Lexer lexer(source, filename);
    
    std::cout << "=== Tokens for " << filename << " ===\n\n";
    
    auto tokens = lexer.tokenize_all();
    for (const auto& token : tokens) {
        std::cout << token << "\n";
    }
    
    if (lexer.has_errors()) {
        std::cout << "\n=== Lexer Errors ===\n";
        for (const auto& error : lexer.errors()) {
            std::cout << filename << ":" << error.location.line << ":" 
                      << error.location.column << ": error: " << error.lexeme << "\n";
        }
        return 1;
    }
    
    std::cout << "\nTotal: " << tokens.size() << " tokens\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string arg1 = argv[1];
    
    if (arg1 == "--help" || arg1 == "-h") {
        print_usage(argv[0]);
        return 0;
    }
    
    if (arg1 == "--version" || arg1 == "-v") {
        std::cout << "Axiom 0.1.0\n";
        return 0;
    }
    
    if (arg1 == "--lex") {
        if (argc < 3) {
            std::cerr << "Error: --lex requires a filename\n";
            return 1;
        }
        std::string source = read_file(argv[2]);
        if (source.empty()) return 1;
        return run_lexer(source, argv[2]);
    }
    
    // Default: try to compile the file
    std::string source = read_file(arg1);
    if (source.empty()) return 1;
    
    // For now, just run the lexer
    std::cout << "Note: Full compilation not yet implemented. Running lexer only.\n\n";
    return run_lexer(source, arg1);
}
