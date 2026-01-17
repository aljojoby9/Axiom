/**
 * @file main.cpp
 * @brief Axiom compiler and REPL entry point
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/type_checker.hpp"
#include "codegen/codegen.hpp"
#include "repl/repl.hpp"

void print_usage(const char* program_name) {
    std::cout << R"(
Axiom Programming Language v0.1.0

USAGE:
    axiom [FLAGS] [OPTIONS] [FILE]

FLAGS:
    -h, --help       Show this help message
    -v, --version    Show version information

COMMANDS:
    axiom                     Start interactive REPL
    axiom <file.ax>           Compile and run a file
    axiom repl                Start interactive REPL
    axiom check <file.ax>     Type-check without compiling
    axiom parse <file.ax>     Parse and show AST info
    axiom lex <file.ax>       Tokenize and show tokens
    axiom emit-ir <file.ax>   Emit LLVM IR

EXAMPLES:
    axiom                     # Start REPL
    axiom hello.ax            # Compile and run
    axiom check mymodule.ax   # Type-check only
    axiom emit-ir main.ax     # Show LLVM IR

)" << std::endl;
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "\033[31merror\033[0m: Could not open file '" << path << "'\n";
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
        std::cout << "\n\033[31m=== Lexer Errors ===\033[0m\n";
        for (const auto& error : lexer.errors()) {
            std::cout << filename << ":" << error.location.line << ":" 
                      << error.location.column << ": \033[31merror\033[0m: " << error.lexeme << "\n";
        }
        return 1;
    }
    
    std::cout << "\n\033[32m✓ " << tokens.size() << " tokens\033[0m\n";
    return 0;
}

int run_parser(const std::string& source, const std::string& filename) {
    axiom::Lexer lexer(source, filename);
    axiom::Parser parser(lexer);
    
    auto program = parser.parse();
    
    if (parser.has_errors()) {
        std::cerr << "\033[31m=== Parse Errors ===\033[0m\n";
        for (const auto& error : parser.errors()) {
            std::cerr << filename << ":" << error.location.line << ":" 
                      << error.location.column << ": \033[31merror\033[0m: " << error.message << "\n";
        }
        return 1;
    }
    
    std::cout << "=== AST for " << filename << " ===\n\n";
    std::cout << "Declarations: " << program.declarations.size() << "\n";
    
    for (size_t i = 0; i < program.declarations.size(); ++i) {
        auto* decl = program.declarations[i].get();
        std::cout << "  [" << i << "] ";
        
        if (auto* fn = dynamic_cast<axiom::ast::FnDecl*>(decl)) {
            std::cout << "fn " << fn->name << "(" << fn->params.size() << " params)\n";
        } else if (auto* st = dynamic_cast<axiom::ast::StructDecl*>(decl)) {
            std::cout << "struct " << st->name << "\n";
        } else if (auto* en = dynamic_cast<axiom::ast::EnumDecl*>(decl)) {
            std::cout << "enum " << en->name << "\n";
        } else if (auto* cl = dynamic_cast<axiom::ast::ClassDecl*>(decl)) {
            std::cout << "class " << cl->name << "\n";
        } else if (auto* tr = dynamic_cast<axiom::ast::TraitDecl*>(decl)) {
            std::cout << "trait " << tr->name << "\n";
        } else {
            std::cout << "<declaration>\n";
        }
    }
    
    std::cout << "\n\033[32m✓ Parsing successful\033[0m\n";
    return 0;
}

int run_check(const std::string& source, const std::string& filename) {
    axiom::Lexer lexer(source, filename);
    axiom::Parser parser(lexer);
    
    auto program = parser.parse();
    
    if (parser.has_errors()) {
        for (const auto& error : parser.errors()) {
            std::cerr << filename << ":" << error.location.line << ":" 
                      << error.location.column << ": \033[31merror\033[0m: " << error.message << "\n";
        }
        return 1;
    }
    
    axiom::semantic::TypeChecker checker;
    checker.check(program);
    
    if (checker.has_errors()) {
        std::cerr << "\033[31m=== Type Errors ===\033[0m\n";
        for (const auto& error : checker.errors()) {
            std::cerr << filename << ":" << error.location.line << ":" 
                      << error.location.column << ": \033[31merror\033[0m: " << error.message << "\n";
        }
        return 1;
    }
    
    std::cout << "\033[32m✓ Type check passed\033[0m\n";
    return 0;
}

int run_emit_ir(const std::string& source, const std::string& filename) {
    axiom::Lexer lexer(source, filename);
    axiom::Parser parser(lexer);
    
    auto program = parser.parse();
    
    if (parser.has_errors()) {
        for (const auto& error : parser.errors()) {
            std::cerr << filename << ":" << error.location.line << ":" 
                      << error.location.column << ": \033[31merror\033[0m: " << error.message << "\n";
        }
        return 1;
    }
    
    axiom::semantic::TypeChecker checker;
    checker.check(program);
    
    if (checker.has_errors()) {
        for (const auto& error : checker.errors()) {
            std::cerr << filename << ":" << error.location.line << ":" 
                      << error.location.column << ": \033[31merror\033[0m: " << error.message << "\n";
        }
        return 1;
    }
    
    axiom::codegen::initialize_llvm();
    axiom::codegen::CodeGenerator codegen(filename);
    
    if (!codegen.generate(program, checker)) {
        std::cerr << "\033[31m=== Codegen Errors ===\033[0m\n";
        for (const auto& error : codegen.errors()) {
            std::cerr << filename << ": \033[31merror\033[0m: " << error.message << "\n";
        }
        return 1;
    }
    
    std::cout << "; ModuleID = '" << filename << "'\n";
    codegen.dump_ir();
    return 0;
}

int run_compile(const std::string& source, const std::string& filename) {
    axiom::Lexer lexer(source, filename);
    axiom::Parser parser(lexer);
    
    auto program = parser.parse();
    
    if (parser.has_errors()) {
        for (const auto& error : parser.errors()) {
            std::cerr << filename << ":" << error.location.line << ":" 
                      << error.location.column << ": \033[31merror\033[0m: " << error.message << "\n";
        }
        return 1;
    }
    
    axiom::semantic::TypeChecker checker;
    checker.check(program);
    
    if (checker.has_errors()) {
        for (const auto& error : checker.errors()) {
            std::cerr << filename << ":" << error.location.line << ":" 
                      << error.location.column << ": \033[31merror\033[0m: " << error.message << "\n";
        }
        return 1;
    }
    
    axiom::codegen::initialize_llvm();
    axiom::codegen::CodeGenerator codegen(filename);
    
    if (!codegen.generate(program, checker)) {
        for (const auto& error : codegen.errors()) {
            std::cerr << filename << ": \033[31merror\033[0m: " << error.message << "\n";
        }
        return 1;
    }
    
    std::cout << "\033[32m✓ Compilation successful\033[0m\n";
    std::cout << "  Declarations: " << program.declarations.size() << "\n";
    
    // TODO: Write object file and link
    std::cout << "\n\033[33mnote\033[0m: Execution not yet implemented. Use 'emit-ir' to see generated LLVM IR.\n";
    
    return 0;
}

int main(int argc, char* argv[]) {
    // No arguments - start REPL
    if (argc < 2) {
        axiom::repl::Repl repl;
        repl.run();
        return 0;
    }
    
    std::string arg1 = argv[1];
    
    // Help
    if (arg1 == "--help" || arg1 == "-h") {
        print_usage(argv[0]);
        return 0;
    }
    
    // Version
    if (arg1 == "--version" || arg1 == "-v") {
        std::cout << "Axiom 0.1.0\n";
        return 0;
    }
    
    // REPL command
    if (arg1 == "repl") {
        axiom::repl::Repl repl;
        repl.run();
        return 0;
    }
    
    // Lex command
    if (arg1 == "lex" || arg1 == "--lex") {
        if (argc < 3) {
            std::cerr << "\033[31merror\033[0m: 'lex' requires a filename\n";
            return 1;
        }
        std::string source = read_file(argv[2]);
        if (source.empty()) return 1;
        return run_lexer(source, argv[2]);
    }
    
    // Parse command
    if (arg1 == "parse" || arg1 == "--parse") {
        if (argc < 3) {
            std::cerr << "\033[31merror\033[0m: 'parse' requires a filename\n";
            return 1;
        }
        std::string source = read_file(argv[2]);
        if (source.empty()) return 1;
        return run_parser(source, argv[2]);
    }
    
    // Check command
    if (arg1 == "check" || arg1 == "--check") {
        if (argc < 3) {
            std::cerr << "\033[31merror\033[0m: 'check' requires a filename\n";
            return 1;
        }
        std::string source = read_file(argv[2]);
        if (source.empty()) return 1;
        return run_check(source, argv[2]);
    }
    
    // Emit IR command
    if (arg1 == "emit-ir" || arg1 == "--emit-ir") {
        if (argc < 3) {
            std::cerr << "\033[31merror\033[0m: 'emit-ir' requires a filename\n";
            return 1;
        }
        std::string source = read_file(argv[2]);
        if (source.empty()) return 1;
        return run_emit_ir(source, argv[2]);
    }
    
    // Default: compile file
    std::string source = read_file(arg1);
    if (source.empty()) return 1;
    return run_compile(source, arg1);
}

