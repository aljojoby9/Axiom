#pragma once

/**
 * @file symbol_table.hpp
 * @brief Symbol table for Axiom semantic analysis
 * 
 * Manages scopes and symbol lookups for variables, functions, and types.
 */

#include "types.hpp"
#include "../parser/ast.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>

namespace axiom {
namespace semantic {

/**
 * @brief Kind of symbol
 */
enum class SymbolKind {
    Variable,
    Function,
    Parameter,
    Type,       // struct, class, enum
    Trait,
    Module,
    EnumVariant,
};

/**
 * @brief A symbol in the symbol table
 */
struct Symbol {
    std::string name;
    SymbolKind kind;
    TypePtr type;
    bool is_mutable = false;
    bool is_public = false;
    bool is_initialized = false;
    SourceLocation definition_loc;
    
    // For functions
    std::vector<std::string> type_params;
    
    Symbol(std::string n, SymbolKind k, TypePtr t)
        : name(std::move(n)), kind(k), type(std::move(t)) {}
};

using SymbolPtr = std::shared_ptr<Symbol>;

/**
 * @brief A scope in the symbol table
 */
class Scope {
public:
    enum class Kind {
        Global,
        Module,
        Function,
        Block,
        Loop,
        Struct,
        Class,
        Trait,
        Impl,
    };
    
    explicit Scope(Kind k, Scope* parent = nullptr)
        : kind_(k), parent_(parent) {}
    
    /**
     * @brief Define a symbol in this scope
     * @return false if symbol already exists
     */
    bool define(SymbolPtr symbol);
    
    /**
     * @brief Look up a symbol in this scope only
     */
    SymbolPtr lookup_local(const std::string& name) const;
    
    /**
     * @brief Look up a symbol in this scope and all parent scopes
     */
    SymbolPtr lookup(const std::string& name) const;
    
    /**
     * @brief Get all symbols defined in this scope
     */
    const std::unordered_map<std::string, SymbolPtr>& symbols() const { 
        return symbols_; 
    }
    
    Kind kind() const { return kind_; }
    Scope* parent() const { return parent_; }
    
    // For loop scopes - track break/continue validity
    bool is_in_loop() const;
    
    // For function scopes - track return type
    TypePtr expected_return_type;
    bool has_return = false;

private:
    Kind kind_;
    Scope* parent_;
    std::unordered_map<std::string, SymbolPtr> symbols_;
};

/**
 * @brief Symbol table managing all scopes
 */
class SymbolTable {
public:
    SymbolTable();
    
    /**
     * @brief Enter a new scope
     */
    void enter_scope(Scope::Kind kind);
    
    /**
     * @brief Exit the current scope
     */
    void exit_scope();
    
    /**
     * @brief Define a symbol in the current scope
     */
    bool define(const std::string& name, SymbolKind kind, TypePtr type);
    
    /**
     * @brief Define a symbol with full control
     */
    bool define(SymbolPtr symbol);
    
    /**
     * @brief Look up a symbol by name (searches all scopes)
     */
    SymbolPtr lookup(const std::string& name) const;
    
    /**
     * @brief Look up a symbol in current scope only
     */
    SymbolPtr lookup_local(const std::string& name) const;
    
    /**
     * @brief Look up a type by name
     */
    TypePtr lookup_type(const std::string& name) const;
    
    /**
     * @brief Register a type in the type registry
     */
    void register_type(const std::string& name, TypePtr type);
    
    /**
     * @brief Get current scope
     */
    Scope* current_scope() const { return scopes_.back().get(); }
    
    /**
     * @brief Get global scope
     */
    Scope* global_scope() const { return scopes_.front().get(); }
    
    /**
     * @brief Check if currently inside a loop
     */
    bool in_loop() const;
    
    /**
     * @brief Check if currently inside a function
     */
    bool in_function() const;
    
    /**
     * @brief Get current function's expected return type
     */
    TypePtr current_return_type() const;
    
    /**
     * @brief Set that current function has a return statement
     */
    void set_has_return();

private:
    std::vector<std::unique_ptr<Scope>> scopes_;
    std::unordered_map<std::string, TypePtr> type_registry_;
    
    // Initialize built-in types
    void init_builtins();
};

} // namespace semantic
} // namespace axiom
