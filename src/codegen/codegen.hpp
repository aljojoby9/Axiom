#pragma once

/**
 * @file codegen.hpp
 * @brief LLVM IR code generation for Axiom
 * 
 * Generates LLVM IR from the typed AST.
 */

#include "../parser/ast.hpp"
#include "../semantic/type_checker.hpp"
#include "../semantic/types.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace axiom {
namespace codegen {

/**
 * @brief Code generation error
 */
struct CodeGenError {
    std::string message;
    SourceLocation location;
    
    CodeGenError(std::string msg, SourceLocation loc = {})
        : message(std::move(msg)), location(loc) {}
};

/**
 * @brief LLVM IR code generator for Axiom
 */
class CodeGenerator {
public:
    CodeGenerator(const std::string& module_name = "axiom_module");
    
    /**
     * @brief Generate IR for an entire program
     */
    bool generate(ast::Program& program, semantic::TypeChecker& type_checker);
    
    /**
     * @brief Get the generated LLVM module
     */
    llvm::Module* get_module() { return module_.get(); }
    
    /**
     * @brief Print generated IR to stdout
     */
    void dump_ir();
    
    /**
     * @brief Write IR to file
     */
    bool write_ir(const std::string& filename);
    
    /**
     * @brief Run optimization passes on the module
     * @param level 0=none, 1=basic, 2=default, 3=aggressive
     */
    void optimize(int level);
    
    /**
     * @brief Compile to object file
     * @param filename Output file path
     * @param opt_level Optimization level (0-3)
     */
    bool compile_to_object(const std::string& filename, int opt_level = 0);
    
    /**
     * @brief Check for errors
     */
    bool has_errors() const { return !errors_.empty(); }
    
    /**
     * @brief Get all errors
     */
    const std::vector<CodeGenError>& errors() const { return errors_; }

private:
    // LLVM core components
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    
    // Type checker reference
    semantic::TypeChecker* type_checker_ = nullptr;
    
    // Symbol tables for LLVM values
    std::unordered_map<std::string, llvm::Value*> named_values_;
    std::unordered_map<std::string, llvm::Function*> functions_;
    std::unordered_map<std::string, llvm::StructType*> struct_types_;
    
    // Current function being generated
    llvm::Function* current_function_ = nullptr;
    
    // Break/continue targets for loops
    std::vector<llvm::BasicBlock*> break_targets_;
    std::vector<llvm::BasicBlock*> continue_targets_;
    
    // Errors
    std::vector<CodeGenError> errors_;
    
    // === Type Conversion ===
    llvm::Type* to_llvm_type(semantic::TypePtr type);
    llvm::Type* to_llvm_type(semantic::TypeKind kind);
    
    // === Declaration Generation ===
    void gen_declaration(ast::Decl* decl);
    llvm::Function* gen_function(ast::FnDecl* fn);
    void gen_struct(ast::StructDecl* st);
    void gen_enum(ast::EnumDecl* en);
    
    // === Statement Generation ===
    void gen_statement(ast::Stmt* stmt);
    void gen_block(ast::Block* block);
    void gen_var_decl(ast::VarDeclStmt* var);
    void gen_if_stmt(ast::IfStmt* if_stmt);
    void gen_while_stmt(ast::WhileStmt* while_stmt);
    void gen_for_stmt(ast::ForStmt* for_stmt);
    void gen_return_stmt(ast::ReturnStmt* ret);
    
    // === Expression Generation ===
    llvm::Value* gen_expr(ast::Expr* expr);
    llvm::Value* gen_literal(ast::Expr* expr);
    llvm::Value* gen_identifier(ast::Identifier* id);
    llvm::Value* gen_binary(ast::BinaryExpr* binary);
    llvm::Value* gen_unary(ast::UnaryExpr* unary);
    llvm::Value* gen_call(ast::CallExpr* call);
    llvm::Value* gen_index(ast::IndexExpr* index);
    llvm::Value* gen_member(ast::MemberExpr* member);
    llvm::Value* gen_assign(ast::AssignExpr* assign);
    
    // === Builtin Functions ===
    void declare_builtins();
    llvm::Function* get_printf();
    
    // === Helpers ===
    void error(const std::string& msg, const SourceLocation& loc = {});
    llvm::AllocaInst* create_alloca(llvm::Function* fn, 
                                     const std::string& name,
                                     llvm::Type* type);
};

/**
 * @brief Initialize LLVM targets (call once at startup)
 */
void initialize_llvm();

} // namespace codegen
} // namespace axiom
