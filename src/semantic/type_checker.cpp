/**
 * @file type_checker.cpp
 * @brief Type checker implementation for Axiom
 */

#include "type_checker.hpp"
#include "types.hpp"
#include "symbol_table.hpp"
#include "../parser/ast.hpp"
#include "../lexer/token.hpp"
#include <sstream>
#include <algorithm>

namespace axiom {
namespace semantic {

TypeChecker::TypeChecker() = default;

TypePtr TypeChecker::fresh_type_var() {
    return std::make_shared<TypeVariable>(next_type_var_id_++);
}

// === Main Entry ===

void TypeChecker::check(ast::Program& program) {
    // First pass: register all type declarations
    for (auto& decl : program.declarations) {
        if (auto* st = dynamic_cast<ast::StructDecl*>(decl.get())) {
            auto type = std::make_shared<StructType>(st->name);
            symbols_.register_type(st->name, type);
        } else if (auto* cls = dynamic_cast<ast::ClassDecl*>(decl.get())) {
            auto type = std::make_shared<ClassType>(cls->name);
            symbols_.register_type(cls->name, type);
        } else if (auto* en = dynamic_cast<ast::EnumDecl*>(decl.get())) {
            auto type = std::make_shared<EnumType>(en->name);
            symbols_.register_type(en->name, type);
        } else if (auto* trait = dynamic_cast<ast::TraitDecl*>(decl.get())) {
            auto type = std::make_shared<TraitType>(trait->name);
            symbols_.register_type(trait->name, type);
        }
    }
    
    // Second pass: check all declarations
    for (auto& decl : program.declarations) {
        check_declaration(decl.get());
    }
}

// === Declaration Checking ===

void TypeChecker::check_declaration(ast::Decl* decl) {
    if (auto* fn = dynamic_cast<ast::FnDecl*>(decl)) {
        check_function(fn);
    } else if (auto* st = dynamic_cast<ast::StructDecl*>(decl)) {
        check_struct(st);
    } else if (auto* cls = dynamic_cast<ast::ClassDecl*>(decl)) {
        check_class(cls);
    } else if (auto* trait = dynamic_cast<ast::TraitDecl*>(decl)) {
        check_trait(trait);
    } else if (auto* impl = dynamic_cast<ast::ImplDecl*>(decl)) {
        check_impl(impl);
    } else if (auto* en = dynamic_cast<ast::EnumDecl*>(decl)) {
        check_enum(en);
    } else if (auto* alias = dynamic_cast<ast::TypeAliasDecl*>(decl)) {
        check_type_alias(alias);
    } else if (auto* import = dynamic_cast<ast::ImportDecl*>(decl)) {
        check_import(import);
    }
}

void TypeChecker::check_function(ast::FnDecl* fn) {
    current_function_ = fn;
    
    // Resolve parameter types
    std::vector<TypePtr> param_types;
    for (const auto& param : fn->params) {
        TypePtr ptype = param.type ? resolve_type(param.type.get()) : unknown_type();
        param_types.push_back(ptype);
    }
    
    // Resolve return type
    TypePtr return_type = fn->return_type 
        ? resolve_type(fn->return_type->get()) 
        : void_type();
    
    // Create function type
    auto fn_type = std::make_shared<FunctionType>(param_types, return_type);
    fn_type->is_async = fn->is_async;
    
    // Register function in symbol table
    auto fn_sym = std::make_shared<Symbol>(fn->name, SymbolKind::Function, fn_type);
    fn_sym->is_public = fn->is_public;
    fn_sym->definition_loc = fn->location;
    fn_sym->is_initialized = true;
    fn_sym->type_params = fn->type_params;
    
    if (!symbols_.define(fn_sym)) {
        error_redefinition(fn->name, fn->location);
    }
    
    // Enter function scope
    symbols_.enter_scope(Scope::Kind::Function);
    symbols_.current_scope()->expected_return_type = return_type;
    
    // Define parameters
    for (size_t i = 0; i < fn->params.size(); ++i) {
        auto& param = fn->params[i];
        auto param_sym = std::make_shared<Symbol>(
            param.name, SymbolKind::Parameter, param_types[i]
        );
        param_sym->is_mutable = param.is_mutable;
        param_sym->is_initialized = true;
        symbols_.define(param_sym);
    }
    
    // Check function body
    if (fn->body) {
        check_block(fn->body.get());
    }
    
    // Check for return statement if non-void
    if (return_type->kind != TypeKind::Void && 
        !symbols_.current_scope()->has_return) {
        error("Function '" + fn->name + "' must return a value", fn->location);
    }
    
    symbols_.exit_scope();
    current_function_ = nullptr;
}

void TypeChecker::check_struct(ast::StructDecl* st) {
    current_struct_ = st;
    
    // Get registered type
    auto type = std::dynamic_pointer_cast<StructType>(
        symbols_.lookup_type(st->name)
    );
    if (!type) return;
    
    // Enter struct scope for methods
    symbols_.enter_scope(Scope::Kind::Struct);
    
    // Register generic type params
    for (const auto& tp : st->type_params) {
        type->type_params.push_back(tp);
        auto gen = std::make_shared<GenericType>(tp);
        symbols_.register_type(tp, gen);
    }
    
    // Check and add fields
    for (auto& field : st->fields) {
        TypePtr field_type = resolve_type(field.type.get());
        StructField sf;
        sf.name = field.name;
        sf.type = field_type;
        sf.is_public = field.is_public;
        type->fields.push_back(sf);
    }
    
    // Check methods
    for (auto& method : st->methods) {
        check_function(method.get());
    }
    
    symbols_.exit_scope();
    current_struct_ = nullptr;
}

void TypeChecker::check_class(ast::ClassDecl* cls) {
    current_class_ = cls;
    
    auto type = std::dynamic_pointer_cast<ClassType>(
        symbols_.lookup_type(cls->name)
    );
    if (!type) return;
    
    type->base_class = cls->base_class;
    
    symbols_.enter_scope(Scope::Kind::Class);
    
    // Check fields
    for (auto& field : cls->fields) {
        TypePtr field_type = resolve_type(field.type.get());
        StructField sf;
        sf.name = field.name;
        sf.type = field_type;
        sf.is_public = field.is_public;
        type->fields.push_back(sf);
    }
    
    // Check methods
    for (auto& method : cls->methods) {
        check_function(method.get());
    }
    
    symbols_.exit_scope();
    current_class_ = nullptr;
}

void TypeChecker::check_trait(ast::TraitDecl* trait) {
    auto type = std::dynamic_pointer_cast<TraitType>(
        symbols_.lookup_type(trait->name)
    );
    if (!type) return;
    
    symbols_.enter_scope(Scope::Kind::Trait);
    
    // Register type params
    for (const auto& tp : trait->type_params) {
        type->type_params.push_back(tp);
    }
    
    // Check method signatures
    for (auto& method : trait->methods) {
        check_function(method.get());
    }
    
    symbols_.exit_scope();
}

void TypeChecker::check_impl(ast::ImplDecl* impl) {
    symbols_.enter_scope(Scope::Kind::Impl);
    
    // Check methods
    for (auto& method : impl->methods) {
        check_function(method.get());
    }
    
    symbols_.exit_scope();
}

void TypeChecker::check_enum(ast::EnumDecl* en) {
    auto type = std::dynamic_pointer_cast<EnumType>(
        symbols_.lookup_type(en->name)
    );
    if (!type) return;
    
    // Register variants
    for (const auto& variant : en->variants) {
        EnumVariant ev;
        ev.name = variant.name;
        for (const auto& field : variant.fields) {
            ev.fields.push_back(resolve_type(field.get()));
        }
        type->variants.push_back(ev);
        
        // Register variant as a constructor function
        std::vector<TypePtr> param_types;
        for (const auto& f : ev.fields) {
            param_types.push_back(f);
        }
        auto ctor_type = std::make_shared<FunctionType>(param_types, type);
        auto ctor_sym = std::make_shared<Symbol>(
            en->name + "::" + variant.name, 
            SymbolKind::EnumVariant, 
            ctor_type
        );
        symbols_.define(ctor_sym);
    }
}

void TypeChecker::check_type_alias(ast::TypeAliasDecl* alias) {
    TypePtr aliased = resolve_type(alias->aliased_type.get());
    symbols_.register_type(alias->name, aliased);
}

void TypeChecker::check_import(ast::ImportDecl* /*import*/) {
    // Import handling - would load module symbols
    // For now, just accept it
}

// === Statement Checking ===

void TypeChecker::check_statement(ast::Stmt* stmt) {
    if (auto* var = dynamic_cast<ast::VarDeclStmt*>(stmt)) {
        check_var_decl(var);
    } else if (auto* if_stmt = dynamic_cast<ast::IfStmt*>(stmt)) {
        check_if_stmt(if_stmt);
    } else if (auto* while_stmt = dynamic_cast<ast::WhileStmt*>(stmt)) {
        check_while_stmt(while_stmt);
    } else if (auto* for_stmt = dynamic_cast<ast::ForStmt*>(stmt)) {
        check_for_stmt(for_stmt);
    } else if (auto* match_stmt = dynamic_cast<ast::MatchStmt*>(stmt)) {
        check_match_stmt(match_stmt);
    } else if (auto* ret = dynamic_cast<ast::ReturnStmt*>(stmt)) {
        check_return_stmt(ret);
    } else if (auto* brk = dynamic_cast<ast::BreakStmt*>(stmt)) {
        if (!symbols_.in_loop()) {
            error("'break' outside of loop", brk->location);
        }
    } else if (auto* cont = dynamic_cast<ast::ContinueStmt*>(stmt)) {
        if (!symbols_.in_loop()) {
            error("'continue' outside of loop", cont->location);
        }
    } else if (auto* expr_stmt = dynamic_cast<ast::ExprStmt*>(stmt)) {
        infer_type(expr_stmt->expression.get());
    }
}

void TypeChecker::check_block(ast::Block* block) {
    symbols_.enter_scope(Scope::Kind::Block);
    
    for (auto& stmt : block->statements) {
        check_statement(stmt.get());
    }
    
    symbols_.exit_scope();
}

void TypeChecker::check_var_decl(ast::VarDeclStmt* var) {
    TypePtr declared_type = var->type 
        ? resolve_type((*var->type).get()) 
        : nullptr;
    
    TypePtr init_type = nullptr;
    if (var->initializer) {
        init_type = infer_type(var->initializer->get());
    }
    
    // Determine final type
    TypePtr var_type;
    if (declared_type && init_type) {
        if (!is_assignable(init_type, declared_type)) {
            error_type_mismatch(declared_type, init_type, var->location);
        }
        var_type = declared_type;
    } else if (declared_type) {
        var_type = declared_type;
    } else if (init_type) {
        var_type = init_type;
    } else {
        error("Cannot determine type of '" + var->name + "'", var->location);
        var_type = unknown_type();
    }
    
    // Register variable
    auto sym = std::make_shared<Symbol>(var->name, SymbolKind::Variable, var_type);
    sym->is_mutable = var->is_mutable;
    sym->is_initialized = var->initializer.has_value();
    sym->definition_loc = var->location;
    
    if (!symbols_.define(sym)) {
        error_redefinition(var->name, var->location);
    }
}

void TypeChecker::check_if_stmt(ast::IfStmt* if_stmt) {
    TypePtr cond_type = infer_type(if_stmt->condition.get());
    if (cond_type->kind != TypeKind::Bool) {
        error("Condition must be bool", if_stmt->condition->location);
    }
    
    check_block(if_stmt->then_block.get());
    
    for (auto& elif : if_stmt->elif_blocks) {
        TypePtr elif_cond = infer_type(elif.first.get());
        if (elif_cond->kind != TypeKind::Bool) {
            error("Condition must be bool", elif.first->location);
        }
        check_block(elif.second.get());
    }
    
    if (if_stmt->else_block) {
        check_block((*if_stmt->else_block).get());
    }
}

void TypeChecker::check_while_stmt(ast::WhileStmt* while_stmt) {
    TypePtr cond_type = infer_type(while_stmt->condition.get());
    if (cond_type->kind != TypeKind::Bool) {
        error("Condition must be bool", while_stmt->condition->location);
    }
    
    symbols_.enter_scope(Scope::Kind::Loop);
    check_block(while_stmt->body.get());
    symbols_.exit_scope();
}

void TypeChecker::check_for_stmt(ast::ForStmt* for_stmt) {
    TypePtr iter_type = infer_type(for_stmt->iterable.get());
    
    // Determine element type from iterable
    TypePtr elem_type;
    if (iter_type->kind == TypeKind::List) {
        elem_type = std::static_pointer_cast<ListType>(iter_type)->element_type;
    } else if (iter_type->kind == TypeKind::Array) {
        elem_type = std::static_pointer_cast<ArrayType>(iter_type)->element_type;
    } else {
        // Assume i64 for ranges
        elem_type = i64_type();
    }
    
    symbols_.enter_scope(Scope::Kind::Loop);
    
    auto var_sym = std::make_shared<Symbol>(
        for_stmt->variable, SymbolKind::Variable, elem_type
    );
    var_sym->is_mutable = false;
    var_sym->is_initialized = true;
    symbols_.define(var_sym);
    
    check_block(for_stmt->body.get());
    
    symbols_.exit_scope();
}

void TypeChecker::check_match_stmt(ast::MatchStmt* match_stmt) {
    TypePtr match_type = infer_type(match_stmt->value.get());
    
    for (auto& arm : match_stmt->arms) {
        // Check pattern type matches
        TypePtr pattern_type = infer_type(arm.pattern.get());
        if (!is_assignable(match_type, pattern_type)) {
            // Allow - patterns might be more general
        }
        
        if (arm.guard) {
            TypePtr guard_type = infer_type(arm.guard->get());
            if (guard_type->kind != TypeKind::Bool) {
                error("Match guard must be bool", arm.guard->get()->location);
            }
        }
        
        check_block(arm.body.get());
    }
}

void TypeChecker::check_return_stmt(ast::ReturnStmt* ret) {
    if (!symbols_.in_function()) {
        error("'return' outside of function", ret->location);
        return;
    }
    
    symbols_.set_has_return();
    
    TypePtr expected = symbols_.current_return_type();
    if (ret->value) {
        TypePtr actual = infer_type(ret->value->get());
        if (!is_assignable(actual, expected)) {
            error_type_mismatch(expected, actual, ret->location);
        }
    } else if (expected && expected->kind != TypeKind::Void) {
        error("Expected return value of type " + expected->to_string(), 
              ret->location);
    }
}

// === Expression Type Inference ===

TypePtr TypeChecker::infer_type(ast::Expr* expr) {
    if (!expr) return unknown_type();
    
    if (auto* lit = dynamic_cast<ast::IntLiteral*>(expr)) {
        return i64_type();
    }
    if (auto* lit = dynamic_cast<ast::FloatLiteral*>(expr)) {
        return f64_type();
    }
    if (auto* lit = dynamic_cast<ast::StringLiteral*>(expr)) {
        return string_type();
    }
    if (auto* lit = dynamic_cast<ast::BoolLiteral*>(expr)) {
        return bool_type();
    }
    if (dynamic_cast<ast::NoneLiteral*>(expr)) {
        return std::make_shared<OptionalType>(unknown_type());
    }
    if (auto* id = dynamic_cast<ast::Identifier*>(expr)) {
        return infer_identifier(id);
    }
    if (auto* bin = dynamic_cast<ast::BinaryExpr*>(expr)) {
        return infer_binary(bin);
    }
    if (auto* un = dynamic_cast<ast::UnaryExpr*>(expr)) {
        return infer_unary(un);
    }
    if (auto* call = dynamic_cast<ast::CallExpr*>(expr)) {
        return infer_call(call);
    }
    if (auto* idx = dynamic_cast<ast::IndexExpr*>(expr)) {
        return infer_index(idx);
    }
    if (auto* mem = dynamic_cast<ast::MemberExpr*>(expr)) {
        return infer_member(mem);
    }
    if (auto* lambda = dynamic_cast<ast::LambdaExpr*>(expr)) {
        return infer_lambda(lambda);
    }
    if (auto* list = dynamic_cast<ast::ListExpr*>(expr)) {
        return infer_list(list);
    }
    if (auto* dict = dynamic_cast<ast::DictExpr*>(expr)) {
        return infer_dict(dict);
    }
    if (auto* tuple = dynamic_cast<ast::TupleExpr*>(expr)) {
        return infer_tuple(tuple);
    }
    if (auto* comp = dynamic_cast<ast::ListCompExpr*>(expr)) {
        return infer_list_comp(comp);
    }
    if (auto* assign = dynamic_cast<ast::AssignExpr*>(expr)) {
        return infer_assign(assign);
    }
    if (auto* range = dynamic_cast<ast::RangeExpr*>(expr)) {
        return infer_range(range);
    }
    if (auto* await = dynamic_cast<ast::AwaitExpr*>(expr)) {
        return infer_await(await);
    }
    
    return unknown_type();
}

TypePtr TypeChecker::infer_identifier(ast::Identifier* id) {
    auto sym = symbols_.lookup(id->name);
    if (!sym) {
        error_undefined(id->name, id->location);
        return unknown_type();
    }
    return sym->type;
}

TypePtr TypeChecker::infer_binary(ast::BinaryExpr* binary) {
    TypePtr left = infer_type(binary->left.get());
    TypePtr right = infer_type(binary->right.get());
    
    switch (binary->op) {
        // Arithmetic - result is common numeric type
        case ast::BinaryOp::Add:
        case ast::BinaryOp::Sub:
        case ast::BinaryOp::Mul:
        case ast::BinaryOp::Div:
        case ast::BinaryOp::Mod:
        case ast::BinaryOp::Pow:
            if (!left->is_numeric()) {
                error("Left operand must be numeric", binary->left->location);
            }
            if (!right->is_numeric()) {
                error("Right operand must be numeric", binary->right->location);
            }
            return common_type(left, right);
        
        // Comparison - result is bool
        case ast::BinaryOp::Eq:
        case ast::BinaryOp::Ne:
        case ast::BinaryOp::Lt:
        case ast::BinaryOp::Le:
        case ast::BinaryOp::Gt:
        case ast::BinaryOp::Ge:
            return bool_type();
        
        // Logical - operands must be bool
        case ast::BinaryOp::And:
        case ast::BinaryOp::Or:
            if (left->kind != TypeKind::Bool) {
                error("Left operand must be bool", binary->left->location);
            }
            if (right->kind != TypeKind::Bool) {
                error("Right operand must be bool", binary->right->location);
            }
            return bool_type();
        
        // Bitwise - operands must be integer
        case ast::BinaryOp::BitAnd:
        case ast::BinaryOp::BitOr:
        case ast::BinaryOp::BitXor:
        case ast::BinaryOp::Shl:
        case ast::BinaryOp::Shr:
            if (!left->is_integer()) {
                error("Left operand must be integer", binary->left->location);
            }
            if (!right->is_integer()) {
                error("Right operand must be integer", binary->right->location);
            }
            return left;
        
        // Matrix multiplication
        case ast::BinaryOp::MatMul:
            return left;  // Tensor types - simplified
    }
    
    return unknown_type();
}

TypePtr TypeChecker::infer_unary(ast::UnaryExpr* unary) {
    TypePtr operand = infer_type(unary->operand.get());
    
    switch (unary->op) {
        case ast::UnaryOp::Neg:
            if (!operand->is_numeric()) {
                error("Operand must be numeric", unary->operand->location);
            }
            return operand;
        
        case ast::UnaryOp::Not:
            if (operand->kind != TypeKind::Bool) {
                error("Operand must be bool", unary->operand->location);
            }
            return bool_type();
        
        case ast::UnaryOp::BitNot:
            if (!operand->is_integer()) {
                error("Operand must be integer", unary->operand->location);
            }
            return operand;
    }
    
    return unknown_type();
}

TypePtr TypeChecker::infer_call(ast::CallExpr* call) {
    TypePtr callee_type = infer_type(call->callee.get());
    
    if (callee_type->kind != TypeKind::Function) {
        error("Cannot call non-function type", call->callee->location);
        return unknown_type();
    }
    
    auto* fn = static_cast<FunctionType*>(callee_type.get());
    
    // Check argument count
    if (call->arguments.size() != fn->param_types.size()) {
        std::ostringstream ss;
        ss << "Expected " << fn->param_types.size() << " arguments, got " 
           << call->arguments.size();
        error(ss.str(), call->location);
    }
    
    // Check argument types
    for (size_t i = 0; i < std::min(call->arguments.size(), fn->param_types.size()); ++i) {
        TypePtr arg_type = infer_type(call->arguments[i].get());
        if (!is_assignable(arg_type, fn->param_types[i])) {
            error_type_mismatch(fn->param_types[i], arg_type, 
                               call->arguments[i]->location);
        }
    }
    
    return fn->return_type;
}

TypePtr TypeChecker::infer_index(ast::IndexExpr* index) {
    TypePtr obj_type = infer_type(index->object.get());
    TypePtr idx_type = infer_type(index->index.get());
    
    if (obj_type->kind == TypeKind::Array) {
        auto* arr = static_cast<ArrayType*>(obj_type.get());
        return arr->element_type;
    }
    if (obj_type->kind == TypeKind::List) {
        auto* list = static_cast<ListType*>(obj_type.get());
        return list->element_type;
    }
    if (obj_type->kind == TypeKind::Dict) {
        auto* dict = static_cast<DictType*>(obj_type.get());
        return dict->value_type;
    }
    if (obj_type->kind == TypeKind::Tuple) {
        auto* tuple = static_cast<TupleType*>(obj_type.get());
        // For constant index, could return exact type
        if (!tuple->element_types.empty()) {
            return tuple->element_types[0];  // Simplified
        }
    }
    if (obj_type->kind == TypeKind::String) {
        return char_type();
    }
    
    error("Cannot index type " + obj_type->to_string(), index->object->location);
    return unknown_type();
}

TypePtr TypeChecker::infer_member(ast::MemberExpr* member) {
    TypePtr obj_type = infer_type(member->object.get());
    
    if (obj_type->kind == TypeKind::Struct) {
        auto* st = static_cast<StructType*>(obj_type.get());
        TypePtr field_type = st->get_field_type(member->member);
        if (!field_type) {
            error("Struct '" + st->name + "' has no field '" + member->member + "'",
                  member->location);
            return unknown_type();
        }
        return field_type;
    }
    
    if (obj_type->kind == TypeKind::Class) {
        auto* cls = static_cast<ClassType*>(obj_type.get());
        TypePtr field_type = cls->get_field_type(member->member);
        if (!field_type) {
            error("Class '" + cls->name + "' has no field '" + member->member + "'",
                  member->location);
            return unknown_type();
        }
        return field_type;
    }
    
    error("Cannot access member on type " + obj_type->to_string(),
          member->object->location);
    return unknown_type();
}

TypePtr TypeChecker::infer_lambda(ast::LambdaExpr* lambda) {
    std::vector<TypePtr> param_types;
    
    symbols_.enter_scope(Scope::Kind::Function);
    
    for (const auto& param : lambda->params) {
        TypePtr ptype = param.type ? resolve_type((*param.type).get()) : fresh_type_var();
        param_types.push_back(ptype);
        
        auto sym = std::make_shared<Symbol>(param.name, SymbolKind::Parameter, ptype);
        sym->is_initialized = true;
        symbols_.define(sym);
    }
    
    TypePtr body_type = infer_type(lambda->body.get());
    
    symbols_.exit_scope();
    
    TypePtr return_type = lambda->return_type 
        ? resolve_type((*lambda->return_type).get()) 
        : body_type;
    
    return std::make_shared<FunctionType>(param_types, return_type);
}

TypePtr TypeChecker::infer_list(ast::ListExpr* list) {
    if (list->elements.empty()) {
        return std::make_shared<ListType>(fresh_type_var());
    }
    
    TypePtr elem_type = infer_type(list->elements[0].get());
    for (size_t i = 1; i < list->elements.size(); ++i) {
        TypePtr t = infer_type(list->elements[i].get());
        elem_type = common_type(elem_type, t);
    }
    
    return std::make_shared<ListType>(elem_type);
}

TypePtr TypeChecker::infer_dict(ast::DictExpr* dict) {
    if (dict->entries.empty()) {
        return std::make_shared<DictType>(fresh_type_var(), fresh_type_var());
    }
    
    TypePtr key_type = infer_type(dict->entries[0].first.get());
    TypePtr val_type = infer_type(dict->entries[0].second.get());
    
    for (size_t i = 1; i < dict->entries.size(); ++i) {
        TypePtr kt = infer_type(dict->entries[i].first.get());
        TypePtr vt = infer_type(dict->entries[i].second.get());
        key_type = common_type(key_type, kt);
        val_type = common_type(val_type, vt);
    }
    
    return std::make_shared<DictType>(key_type, val_type);
}

TypePtr TypeChecker::infer_tuple(ast::TupleExpr* tuple) {
    std::vector<TypePtr> elem_types;
    for (auto& elem : tuple->elements) {
        elem_types.push_back(infer_type(elem.get()));
    }
    return std::make_shared<TupleType>(elem_types);
}

TypePtr TypeChecker::infer_list_comp(ast::ListCompExpr* comp) {
    TypePtr iter_type = infer_type(comp->iterable.get());
    
    TypePtr elem_type;
    if (iter_type->kind == TypeKind::List) {
        elem_type = std::static_pointer_cast<ListType>(iter_type)->element_type;
    } else {
        elem_type = i64_type();  // Assume range
    }
    
    symbols_.enter_scope(Scope::Kind::Block);
    
    auto var_sym = std::make_shared<Symbol>(comp->var_name, SymbolKind::Variable, elem_type);
    var_sym->is_initialized = true;
    symbols_.define(var_sym);
    
    TypePtr result_elem = infer_type(comp->element.get());
    
    if (comp->condition) {
        TypePtr cond_type = infer_type(comp->condition->get());
        if (cond_type->kind != TypeKind::Bool) {
            error("Comprehension condition must be bool", 
                  comp->condition->get()->location);
        }
    }
    
    symbols_.exit_scope();
    
    return std::make_shared<ListType>(result_elem);
}

TypePtr TypeChecker::infer_assign(ast::AssignExpr* assign) {
    TypePtr target_type = infer_type(assign->target.get());
    TypePtr value_type = infer_type(assign->value.get());
    
    // Check target is mutable
    if (auto* id = dynamic_cast<ast::Identifier*>(assign->target.get())) {
        auto sym = symbols_.lookup(id->name);
        if (sym && !sym->is_mutable) {
            error("Cannot assign to immutable variable '" + id->name + "'",
                  assign->location);
        }
    }
    
    if (!is_assignable(value_type, target_type)) {
        error_type_mismatch(target_type, value_type, assign->location);
    }
    
    return target_type;
}

TypePtr TypeChecker::infer_range(ast::RangeExpr* range) {
    TypePtr start = infer_type(range->start.get());
    TypePtr end = infer_type(range->end.get());
    
    if (!start->is_integer()) {
        error("Range start must be integer", range->start->location);
    }
    if (!end->is_integer()) {
        error("Range end must be integer", range->end->location);
    }
    
    // Return an iterable of integers - using List as stand-in for Range
    return std::make_shared<ListType>(i64_type());
}

TypePtr TypeChecker::infer_await(ast::AwaitExpr* await) {
    TypePtr inner = infer_type(await->operand.get());
    // For now, just return the inner type
    // Full impl would unwrap Future/Promise type
    return inner;
}

// === Type Resolution ===

TypePtr TypeChecker::resolve_type(ast::Type* ast_type) {
    if (!ast_type) return unknown_type();
    
    if (auto* simple = dynamic_cast<ast::SimpleType*>(ast_type)) {
        return resolve_simple_type(simple->name);
    }
    
    if (auto* generic = dynamic_cast<ast::GenericType*>(ast_type)) {
        std::vector<TypePtr> args;
        for (auto& arg : generic->type_args) {
            args.push_back(resolve_type(arg.get()));
        }
        return resolve_generic_type(generic->name, args);
    }
    
    if (auto* arr = dynamic_cast<ast::ArrayType*>(ast_type)) {
        return std::make_shared<ArrayType>(
            resolve_type(arr->element_type.get()), arr->size
        );
    }
    
    if (auto* tuple = dynamic_cast<ast::TupleType*>(ast_type)) {
        std::vector<TypePtr> elems;
        for (auto& e : tuple->element_types) {
            elems.push_back(resolve_type(e.get()));
        }
        return std::make_shared<TupleType>(elems);
    }
    
    if (auto* fn = dynamic_cast<ast::FunctionType*>(ast_type)) {
        std::vector<TypePtr> params;
        for (auto& p : fn->param_types) {
            params.push_back(resolve_type(p.get()));
        }
        return std::make_shared<FunctionType>(
            params, resolve_type(fn->return_type.get())
        );
    }
    
    if (auto* ref = dynamic_cast<ast::ReferenceType*>(ast_type)) {
        return std::make_shared<ReferenceType>(
            resolve_type(ref->inner.get()), ref->is_mutable
        );
    }
    
    return unknown_type();
}

TypePtr TypeChecker::resolve_simple_type(const std::string& name) {
    TypePtr type = symbols_.lookup_type(name);
    if (!type) {
        // Maybe it's a generic parameter in scope
        auto sym = symbols_.lookup(name);
        if (sym && sym->kind == SymbolKind::Type) {
            return sym->type;
        }
        return unknown_type();
    }
    return type;
}

TypePtr TypeChecker::resolve_generic_type(const std::string& name,
                                           const std::vector<TypePtr>& args) {
    if (name == "List" && args.size() == 1) {
        return std::make_shared<ListType>(args[0]);
    }
    if (name == "Dict" && args.size() == 2) {
        return std::make_shared<DictType>(args[0], args[1]);
    }
    if (name == "Result" && args.size() == 2) {
        return std::make_shared<ResultType>(args[0], args[1]);
    }
    if (name == "Optional" && args.size() == 1) {
        return std::make_shared<OptionalType>(args[0]);
    }
    
    // User-defined generic type
    TypePtr base = symbols_.lookup_type(name);
    if (base) {
        // Create substitution map and instantiate
        // Simplified for now
        return base->clone();
    }
    
    return unknown_type();
}

// === Error Helpers ===

void TypeChecker::error(const std::string& message, const SourceLocation& loc) {
    errors_.emplace_back(message, loc);
}

void TypeChecker::error_type_mismatch(const TypePtr& expected, const TypePtr& actual,
                                       const SourceLocation& loc) {
    std::string msg = "Type mismatch: expected " + 
                      (expected ? expected->to_string() : "?") + 
                      ", got " + 
                      (actual ? actual->to_string() : "?");
    error(msg, loc);
}

void TypeChecker::error_undefined(const std::string& name, const SourceLocation& loc) {
    error("Undefined symbol '" + name + "'", loc);
}

void TypeChecker::error_redefinition(const std::string& name, const SourceLocation& loc) {
    error("Redefinition of '" + name + "'", loc);
}

} // namespace semantic
} // namespace axiom
