#pragma once

/**
 * @file type_checker.hpp
 * @brief Type checker for Axiom semantic analysis
 * 
 * Performs type checking and inference on the AST.
 */

#include "symbol_table.hpp"
#include "types.hpp"
#include "../parser/ast.hpp"
#include <vector>
#include <string>

namespace axiom {
namespace semantic {

/**
 * @brief Semantic error information
 */
struct SemanticError {
    std::string message;
    SourceLocation location;
    
    SemanticError(std::string msg, SourceLocation loc)
        : message(std::move(msg)), location(loc) {}
};

/**
 * @brief Type checker for Axiom programs
 * 
 * Performs:
 * - Type checking and inference
 * - Scope resolution
 * - Symbol table population
 * - Semantic validation
 */
class TypeChecker {
public:
    TypeChecker();
    
    /**
     * @brief Check an entire program
     */
    void check(ast::Program& program);
    
    /**
     * @brief Check if any errors occurred
     */
    bool has_errors() const { return !errors_.empty(); }
    
    /**
     * @brief Get all semantic errors
     */
    const std::vector<SemanticError>& errors() const { return errors_; }
    
    /**
     * @brief Get the symbol table (for later phases)
     */
    SymbolTable& symbols() { return symbols_; }

private:
    // === Declaration Checking ===
    void check_declaration(ast::Decl* decl);
    void check_function(ast::FnDecl* fn);
    void check_struct(ast::StructDecl* st);
    void check_class(ast::ClassDecl* cls);
    void check_trait(ast::TraitDecl* trait);
    void check_impl(ast::ImplDecl* impl);
    void check_enum(ast::EnumDecl* en);
    void check_type_alias(ast::TypeAliasDecl* alias);
    void check_import(ast::ImportDecl* import);
    
    // === Statement Checking ===
    void check_statement(ast::Stmt* stmt);
    void check_block(ast::Block* block);
    void check_var_decl(ast::VarDeclStmt* var);
    void check_if_stmt(ast::IfStmt* if_stmt);
    void check_while_stmt(ast::WhileStmt* while_stmt);
    void check_for_stmt(ast::ForStmt* for_stmt);
    void check_match_stmt(ast::MatchStmt* match_stmt);
    void check_return_stmt(ast::ReturnStmt* ret);
    
    // === Expression Type Inference ===
    TypePtr infer_type(ast::Expr* expr);
    TypePtr infer_literal(ast::Expr* expr);
    TypePtr infer_identifier(ast::Identifier* id);
    TypePtr infer_binary(ast::BinaryExpr* binary);
    TypePtr infer_unary(ast::UnaryExpr* unary);
    TypePtr infer_call(ast::CallExpr* call);
    TypePtr infer_index(ast::IndexExpr* index);
    TypePtr infer_member(ast::MemberExpr* member);
    TypePtr infer_lambda(ast::LambdaExpr* lambda);
    TypePtr infer_list(ast::ListExpr* list);
    TypePtr infer_dict(ast::DictExpr* dict);
    TypePtr infer_tuple(ast::TupleExpr* tuple);
    TypePtr infer_list_comp(ast::ListCompExpr* comp);
    TypePtr infer_assign(ast::AssignExpr* assign);
    TypePtr infer_range(ast::RangeExpr* range);
    TypePtr infer_await(ast::AwaitExpr* await);
    
    // === Type Resolution ===
    TypePtr resolve_type(ast::Type* ast_type);
    TypePtr resolve_simple_type(const std::string& name);
    TypePtr resolve_generic_type(const std::string& name, 
                                  const std::vector<TypePtr>& args);
    
    // === Error Helpers ===
    void error(const std::string& message, const SourceLocation& loc);
    void error_type_mismatch(const TypePtr& expected, const TypePtr& actual,
                              const SourceLocation& loc);
    void error_undefined(const std::string& name, const SourceLocation& loc);
    void error_redefinition(const std::string& name, const SourceLocation& loc);
    
    // State
    SymbolTable symbols_;
    std::vector<SemanticError> errors_;
    
    // Current context
    ast::FnDecl* current_function_ = nullptr;
    ast::StructDecl* current_struct_ = nullptr;
    ast::ClassDecl* current_class_ = nullptr;
    
    // Type variable counter for inference
    size_t next_type_var_id_ = 0;
    TypePtr fresh_type_var();
};

} // namespace semantic
} // namespace axiom
