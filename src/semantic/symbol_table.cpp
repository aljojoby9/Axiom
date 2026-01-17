/**
 * @file symbol_table.cpp
 * @brief Symbol table implementation for Axiom
 */

#include "symbol_table.hpp"

namespace axiom {
namespace semantic {

// === Scope ===

bool Scope::define(SymbolPtr symbol) {
    if (symbols_.count(symbol->name)) {
        return false;  // Already exists
    }
    symbols_[symbol->name] = std::move(symbol);
    return true;
}

SymbolPtr Scope::lookup_local(const std::string& name) const {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return it->second;
    }
    return nullptr;
}

SymbolPtr Scope::lookup(const std::string& name) const {
    // Check this scope
    auto sym = lookup_local(name);
    if (sym) return sym;
    
    // Check parent scopes
    if (parent_) {
        return parent_->lookup(name);
    }
    
    return nullptr;
}

bool Scope::is_in_loop() const {
    if (kind_ == Kind::Loop) return true;
    if (parent_) return parent_->is_in_loop();
    return false;
}

// === SymbolTable ===

SymbolTable::SymbolTable() {
    // Create global scope
    scopes_.push_back(std::make_unique<Scope>(Scope::Kind::Global));
    init_builtins();
}

void SymbolTable::init_builtins() {
    // Register primitive types
    register_type("void", void_type());
    register_type("bool", bool_type());
    register_type("i8", i8_type());
    register_type("i16", i16_type());
    register_type("i32", i32_type());
    register_type("i64", i64_type());
    register_type("u8", u8_type());
    register_type("u16", u16_type());
    register_type("u32", u32_type());
    register_type("u64", u64_type());
    register_type("f32", f32_type());
    register_type("f64", f64_type());
    register_type("char", char_type());
    register_type("str", string_type());
    
    // Register built-in functions
    // print(value: any) -> void
    auto print_type = std::make_shared<FunctionType>(
        std::vector<TypePtr>{unknown_type()},
        void_type()
    );
    auto print_sym = std::make_shared<Symbol>("print", SymbolKind::Function, print_type);
    print_sym->is_initialized = true;
    define(print_sym);
    
    // len(collection: any) -> i64
    auto len_type = std::make_shared<FunctionType>(
        std::vector<TypePtr>{unknown_type()},
        i64_type()
    );
    auto len_sym = std::make_shared<Symbol>("len", SymbolKind::Function, len_type);
    len_sym->is_initialized = true;
    define(len_sym);
    
    // range(start: i64, end: i64) -> Iterator[i64]
    auto range_type = std::make_shared<FunctionType>(
        std::vector<TypePtr>{i64_type(), i64_type()},
        std::make_shared<ListType>(i64_type())  // Simplified as List
    );
    auto range_sym = std::make_shared<Symbol>("range", SymbolKind::Function, range_type);
    range_sym->is_initialized = true;
    define(range_sym);
    
    // type(value: any) -> str
    auto type_fn = std::make_shared<FunctionType>(
        std::vector<TypePtr>{unknown_type()},
        string_type()
    );
    auto type_sym = std::make_shared<Symbol>("type", SymbolKind::Function, type_fn);
    type_sym->is_initialized = true;
    define(type_sym);
}

void SymbolTable::enter_scope(Scope::Kind kind) {
    auto new_scope = std::make_unique<Scope>(kind, current_scope());
    scopes_.push_back(std::move(new_scope));
}

void SymbolTable::exit_scope() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
    }
}

bool SymbolTable::define(const std::string& name, SymbolKind kind, TypePtr type) {
    auto symbol = std::make_shared<Symbol>(name, kind, std::move(type));
    return define(symbol);
}

bool SymbolTable::define(SymbolPtr symbol) {
    return current_scope()->define(std::move(symbol));
}

SymbolPtr SymbolTable::lookup(const std::string& name) const {
    return current_scope()->lookup(name);
}

SymbolPtr SymbolTable::lookup_local(const std::string& name) const {
    return current_scope()->lookup_local(name);
}

TypePtr SymbolTable::lookup_type(const std::string& name) const {
    auto it = type_registry_.find(name);
    if (it != type_registry_.end()) {
        return it->second;
    }
    return nullptr;
}

void SymbolTable::register_type(const std::string& name, TypePtr type) {
    type_registry_[name] = std::move(type);
}

bool SymbolTable::in_loop() const {
    return current_scope()->is_in_loop();
}

bool SymbolTable::in_function() const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if ((*it)->kind() == Scope::Kind::Function) {
            return true;
        }
    }
    return false;
}

TypePtr SymbolTable::current_return_type() const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if ((*it)->kind() == Scope::Kind::Function) {
            return (*it)->expected_return_type;
        }
    }
    return nullptr;
}

void SymbolTable::set_has_return() {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if ((*it)->kind() == Scope::Kind::Function) {
            (*it)->has_return = true;
            return;
        }
    }
}

} // namespace semantic
} // namespace axiom
