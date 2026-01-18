/**
 * @file driver.cpp
 * @brief Implementation of the compiler driver
 */

#include "driver.hpp"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "../semantic/type_checker.hpp"
#include "../codegen/codegen.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace axiom {

namespace fs = std::filesystem;

Driver::Driver(const CompilerConfig& config) : config_(config) {}

int Driver::run() {
    // Step 1: Compile to object file
    if (config_.emit_obj) {
        int result = compile_to_object();
        if (result != 0) return result;
    }
    
    // Step 2: Link to executable
    if (config_.run_linker && config_.emit_obj) {
        return invoke_linker();
    }
    
    return 0;
}

int Driver::compile_to_object() {
    if (config_.verbose) {
        std::cout << "Compiling " << config_.input_file << "...\n";
    }

    // Read source file
    std::ifstream file(config_.input_file);
    if (!file.is_open()) {
        std::cerr << "\033[31merror\033[0m: Could not open " << config_.input_file << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Lexer
    Lexer lexer(source);
    
    // Parser
    Parser parser(lexer);
    auto program = parser.parse();
    if (parser.has_errors()) {
        for (const auto& err : parser.errors()) {
            std::cerr << config_.input_file << ": \033[31merror\033[0m: " << err.message << "\n";
        }
        return 1;
    }

    // Type Checker
    semantic::TypeChecker checker;
    checker.check(program);
    if (checker.has_errors()) {
        for (const auto& err : checker.errors()) {
            std::cerr << config_.input_file << ": \033[31merror\033[0m: " << err.message << "\n";
        }
        return 1;
    }

    // Code Generation
    codegen::initialize_llvm();
    codegen::CodeGenerator gen(config_.input_file);
    
    if (!gen.generate(program, checker)) {
        for (const auto& err : gen.errors()) {
            std::cerr << config_.input_file << ": \033[31merror\033[0m: " << err.message << "\n";
        }
        return 1;
    }
    
    // Write object file
    std::string obj_file = get_output_filename(".obj");
    if (config_.verbose) {
        std::cout << "Generating " << obj_file;
        if (config_.optimization_level > 0) {
            std::cout << " (optimization level " << config_.optimization_level << ")";
        }
        std::cout << "\n";
    }
    
    if (!gen.compile_to_object(obj_file, config_.optimization_level)) {
        std::cerr << "\033[31merror\033[0m: Failed to generate object file\n";
        return 1;
    }
    
    return 0;
}

int Driver::invoke_linker() {
    std::string linker = find_linker();
    if (linker.empty()) {
        std::cerr << "\033[31merror\033[0m: No linker found (g++ or clang++ required)\n";
        return 1;
    }
    
    std::string obj_file = get_output_filename(".obj");
    std::string exe_file = config_.output_file.empty() 
        ? get_output_filename(".exe") 
        : config_.output_file;
    
    if (config_.verbose) {
        std::cout << "Linking " << exe_file << "...\n";
    }
    
    // Build linker command
    std::string cmd = linker + " -o \"" + exe_file + "\" \"" + obj_file + "\"";
    
    if (config_.verbose) {
        std::cout << "$ " << cmd << "\n";
    }
    
    int result = execute_command(cmd);
    if (result == 0) {
        std::cout << "\033[32m✓ Build successful\033[0m: " << exe_file << "\n";
    }
    return result;
}

std::string Driver::get_output_filename(const std::string& ext) {
    fs::path p(config_.input_file);
    p.replace_extension(ext);
    return p.string();
}

std::string Driver::find_linker() {
    // Check for g++ first (common on MSYS2/MinGW)
#ifdef _WIN32
    if (execute_command("g++ --version > NUL 2>&1") == 0) return "g++";
    if (execute_command("clang++ --version > NUL 2>&1") == 0) return "clang++";
#else
    if (execute_command("g++ --version > /dev/null 2>&1") == 0) return "g++";
    if (execute_command("clang++ --version > /dev/null 2>&1") == 0) return "clang++";
#endif
    return "";
}

int Driver::execute_command(const std::string& cmd) {
    return std::system(cmd.c_str());
}

} // namespace axiom
