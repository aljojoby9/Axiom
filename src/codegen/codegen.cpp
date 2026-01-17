/**
 * @file codegen.cpp
 * @brief LLVM IR code generation implementation for Axiom
 */

#include "codegen.hpp"

namespace axiom {
namespace codegen {

void initialize_llvm() {
    // Initialize only native target to minimize link dependencies
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

CodeGenerator::CodeGenerator(const std::string& module_name) {
    context_ = std::make_unique<llvm::LLVMContext>();
    module_ = std::make_unique<llvm::Module>(module_name, *context_);
    builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
    
    // Set target triple - LLVM 21 requires llvm::Triple type
    module_->setTargetTriple(llvm::Triple(llvm::sys::getDefaultTargetTriple()));
}

// === Main Entry ===

bool CodeGenerator::generate(ast::Program& program, semantic::TypeChecker& type_checker) {
    type_checker_ = &type_checker;
    
    // Declare builtin functions
    declare_builtins();
    
    // First pass: declare all functions (for forward references)
    for (auto& decl : program.declarations) {
        if (auto* fn = dynamic_cast<ast::FnDecl*>(decl.get())) {
            // Get function type from type checker
            auto sym = type_checker.symbols().lookup(fn->name);
            if (!sym) continue;
            
            auto fn_type = std::dynamic_pointer_cast<semantic::FunctionType>(sym->type);
            if (!fn_type) continue;
            
            // Create LLVM function type
            std::vector<llvm::Type*> param_types;
            for (auto& pt : fn_type->param_types) {
                param_types.push_back(to_llvm_type(pt));
            }
            
            auto* ret_type = to_llvm_type(fn_type->return_type);
            auto* llvm_fn_type = llvm::FunctionType::get(ret_type, param_types, false);
            
            auto* llvm_fn = llvm::Function::Create(
                llvm_fn_type,
                llvm::Function::ExternalLinkage,
                fn->name,
                module_.get()
            );
            
            functions_[fn->name] = llvm_fn;
        }
    }
    
    // Second pass: generate function bodies
    for (auto& decl : program.declarations) {
        gen_declaration(decl.get());
    }
    
    // Verify module
    std::string err_str;
    llvm::raw_string_ostream err_stream(err_str);
    if (llvm::verifyModule(*module_, &err_stream)) {
        error("Module verification failed: " + err_str);
        return false;
    }
    
    return !has_errors();
}

// === Output Methods ===

void CodeGenerator::dump_ir() {
    module_->print(llvm::outs(), nullptr);
}

bool CodeGenerator::write_ir(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream out(filename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        error("Could not open file: " + ec.message());
        return false;
    }
    module_->print(out, nullptr);
    return true;
}

bool CodeGenerator::compile_to_object(const std::string& filename) {
    auto target_triple = llvm::sys::getDefaultTargetTriple();
    
    std::string err;
    auto target = llvm::TargetRegistry::lookupTarget(std::string(target_triple), err);
    if (!target) {
        error("Could not get target: " + err);
        return false;
    }
    
    auto cpu = "generic";
    auto features = "";
    
    llvm::TargetOptions opt;
    auto target_machine = target->createTargetMachine(
        std::string(target_triple), cpu, features, opt, std::optional<llvm::Reloc::Model>()
    );
    
    module_->setDataLayout(target_machine->createDataLayout());
    
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        error("Could not open file: " + ec.message());
        return false;
    }
    
    llvm::legacy::PassManager pass;
    auto file_type = llvm::CodeGenFileType::ObjectFile;
    
    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        error("Target machine cannot emit object file");
        return false;
    }
    
    pass.run(*module_);
    dest.flush();
    
    return true;
}

// === Type Conversion ===

llvm::Type* CodeGenerator::to_llvm_type(semantic::TypePtr type) {
    if (!type) return builder_->getVoidTy();
    return to_llvm_type(type->kind);
}

llvm::Type* CodeGenerator::to_llvm_type(semantic::TypeKind kind) {
    using TK = semantic::TypeKind;
    
    switch (kind) {
        case TK::Void:    return builder_->getVoidTy();
        case TK::Bool:    return builder_->getInt1Ty();
        case TK::Int8:    return builder_->getInt8Ty();
        case TK::Int16:   return builder_->getInt16Ty();
        case TK::Int32:   return builder_->getInt32Ty();
        case TK::Int64:   return builder_->getInt64Ty();
        case TK::UInt8:   return builder_->getInt8Ty();
        case TK::UInt16:  return builder_->getInt16Ty();
        case TK::UInt32:  return builder_->getInt32Ty();
        case TK::UInt64:  return builder_->getInt64Ty();
        case TK::Float32: return builder_->getFloatTy();
        case TK::Float64: return builder_->getDoubleTy();
        case TK::Char:    return builder_->getInt8Ty();
        case TK::String:  return builder_->getPtrTy();  // i8*
        default:          return builder_->getInt64Ty();  // Fallback
    }
}

// === Declaration Generation ===

void CodeGenerator::gen_declaration(ast::Decl* decl) {
    if (auto* fn = dynamic_cast<ast::FnDecl*>(decl)) {
        gen_function(fn);
    } else if (auto* st = dynamic_cast<ast::StructDecl*>(decl)) {
        gen_struct(st);
    } else if (auto* en = dynamic_cast<ast::EnumDecl*>(decl)) {
        gen_enum(en);
    }
}

llvm::Function* CodeGenerator::gen_function(ast::FnDecl* fn) {
    auto* func = functions_[fn->name];
    if (!func) {
        error("Function not declared: " + fn->name, fn->location);
        return nullptr;
    }
    
    current_function_ = func;
    named_values_.clear();
    
    // Create entry block
    auto* entry = llvm::BasicBlock::Create(*context_, "entry", func);
    builder_->SetInsertPoint(entry);
    
    // Create allocas for parameters
    size_t idx = 0;
    for (auto& arg : func->args()) {
        auto& param = fn->params[idx];
        auto* alloca = create_alloca(func, param.name, arg.getType());
        builder_->CreateStore(&arg, alloca);
        named_values_[param.name] = alloca;
        arg.setName(param.name);
        ++idx;
    }
    
    // Generate function body
    if (fn->body) {
        gen_block(fn->body.get());
    }
    
    // Add implicit return void if needed
    if (!builder_->GetInsertBlock()->getTerminator()) {
        if (func->getReturnType()->isVoidTy()) {
            builder_->CreateRetVoid();
        } else {
            // Return default value
            builder_->CreateRet(llvm::Constant::getNullValue(func->getReturnType()));
        }
    }
    
    current_function_ = nullptr;
    return func;
}

void CodeGenerator::gen_struct(ast::StructDecl* st) {
    std::vector<llvm::Type*> field_types;
    
    auto sem_type = type_checker_->symbols().lookup_type(st->name);
    if (auto struct_type = std::dynamic_pointer_cast<semantic::StructType>(sem_type)) {
        for (auto& field : struct_type->fields) {
            field_types.push_back(to_llvm_type(field.type));
        }
    }
    
    auto* llvm_struct = llvm::StructType::create(*context_, field_types, st->name);
    struct_types_[st->name] = llvm_struct;
}

void CodeGenerator::gen_enum(ast::EnumDecl* /*en*/) {
    // Enums are represented as integers for now
}

// === Statement Generation ===

void CodeGenerator::gen_statement(ast::Stmt* stmt) {
    if (auto* var = dynamic_cast<ast::VarDeclStmt*>(stmt)) {
        gen_var_decl(var);
    } else if (auto* if_stmt = dynamic_cast<ast::IfStmt*>(stmt)) {
        gen_if_stmt(if_stmt);
    } else if (auto* while_stmt = dynamic_cast<ast::WhileStmt*>(stmt)) {
        gen_while_stmt(while_stmt);
    } else if (auto* for_stmt = dynamic_cast<ast::ForStmt*>(stmt)) {
        gen_for_stmt(for_stmt);
    } else if (auto* ret = dynamic_cast<ast::ReturnStmt*>(stmt)) {
        gen_return_stmt(ret);
    } else if (auto* brk = dynamic_cast<ast::BreakStmt*>(stmt)) {
        if (!break_targets_.empty()) {
            builder_->CreateBr(break_targets_.back());
        }
    } else if (auto* cont = dynamic_cast<ast::ContinueStmt*>(stmt)) {
        if (!continue_targets_.empty()) {
            builder_->CreateBr(continue_targets_.back());
        }
    } else if (auto* expr_stmt = dynamic_cast<ast::ExprStmt*>(stmt)) {
        gen_expr(expr_stmt->expression.get());
    }
}

void CodeGenerator::gen_block(ast::Block* block) {
    for (auto& stmt : block->statements) {
        gen_statement(stmt.get());
        
        // Stop if we hit a terminator
        if (builder_->GetInsertBlock()->getTerminator()) {
            break;
        }
    }
}

void CodeGenerator::gen_var_decl(ast::VarDeclStmt* var) {
    // Get type from semantic analysis
    auto sym = type_checker_->symbols().lookup(var->name);
    llvm::Type* var_type = sym ? to_llvm_type(sym->type) : builder_->getInt64Ty();
    
    auto* alloca = create_alloca(current_function_, var->name, var_type);
    
    if (var->initializer) {
        auto* init_val = gen_expr(var->initializer->get());
        if (init_val) {
            builder_->CreateStore(init_val, alloca);
        }
    }
    
    named_values_[var->name] = alloca;
}

void CodeGenerator::gen_if_stmt(ast::IfStmt* if_stmt) {
    auto* cond = gen_expr(if_stmt->condition.get());
    
    auto* then_bb = llvm::BasicBlock::Create(*context_, "then", current_function_);
    auto* else_bb = llvm::BasicBlock::Create(*context_, "else", current_function_);
    auto* merge_bb = llvm::BasicBlock::Create(*context_, "ifcont", current_function_);
    
    builder_->CreateCondBr(cond, then_bb, else_bb);
    
    // Generate then block
    builder_->SetInsertPoint(then_bb);
    gen_block(if_stmt->then_block.get());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(merge_bb);
    }
    
    // Generate else block
    builder_->SetInsertPoint(else_bb);
    if (if_stmt->else_block) {
        gen_block((*if_stmt->else_block).get());
    }
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(merge_bb);
    }
    
    builder_->SetInsertPoint(merge_bb);
}

void CodeGenerator::gen_while_stmt(ast::WhileStmt* while_stmt) {
    auto* cond_bb = llvm::BasicBlock::Create(*context_, "while.cond", current_function_);
    auto* body_bb = llvm::BasicBlock::Create(*context_, "while.body", current_function_);
    auto* end_bb = llvm::BasicBlock::Create(*context_, "while.end", current_function_);
    
    break_targets_.push_back(end_bb);
    continue_targets_.push_back(cond_bb);
    
    builder_->CreateBr(cond_bb);
    
    // Condition
    builder_->SetInsertPoint(cond_bb);
    auto* cond = gen_expr(while_stmt->condition.get());
    builder_->CreateCondBr(cond, body_bb, end_bb);
    
    // Body
    builder_->SetInsertPoint(body_bb);
    gen_block(while_stmt->body.get());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(cond_bb);
    }
    
    break_targets_.pop_back();
    continue_targets_.pop_back();
    
    builder_->SetInsertPoint(end_bb);
}

void CodeGenerator::gen_for_stmt(ast::ForStmt* for_stmt) {
    // Generate as while loop: for i in 0..n becomes i = 0; while i < n: ... i++
    // Simplified: assume range expression
    auto* start = builder_->getInt64(0);
    auto* end_val = gen_expr(for_stmt->iterable.get());
    
    auto* var_alloca = create_alloca(current_function_, for_stmt->variable, builder_->getInt64Ty());
    builder_->CreateStore(start, var_alloca);
    named_values_[for_stmt->variable] = var_alloca;
    
    auto* cond_bb = llvm::BasicBlock::Create(*context_, "for.cond", current_function_);
    auto* body_bb = llvm::BasicBlock::Create(*context_, "for.body", current_function_);
    auto* incr_bb = llvm::BasicBlock::Create(*context_, "for.incr", current_function_);
    auto* end_bb = llvm::BasicBlock::Create(*context_, "for.end", current_function_);
    
    break_targets_.push_back(end_bb);
    continue_targets_.push_back(incr_bb);
    
    builder_->CreateBr(cond_bb);
    
    // Condition
    builder_->SetInsertPoint(cond_bb);
    auto* current = builder_->CreateLoad(builder_->getInt64Ty(), var_alloca, for_stmt->variable);
    auto* cond = builder_->CreateICmpSLT(current, end_val, "forcond");
    builder_->CreateCondBr(cond, body_bb, end_bb);
    
    // Body
    builder_->SetInsertPoint(body_bb);
    gen_block(for_stmt->body.get());
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(incr_bb);
    }
    
    // Increment
    builder_->SetInsertPoint(incr_bb);
    auto* next = builder_->CreateAdd(
        builder_->CreateLoad(builder_->getInt64Ty(), var_alloca),
        builder_->getInt64(1),
        "nextvar"
    );
    builder_->CreateStore(next, var_alloca);
    builder_->CreateBr(cond_bb);
    
    break_targets_.pop_back();
    continue_targets_.pop_back();
    
    builder_->SetInsertPoint(end_bb);
}

void CodeGenerator::gen_return_stmt(ast::ReturnStmt* ret) {
    if (ret->value) {
        auto* val = gen_expr(ret->value->get());
        builder_->CreateRet(val);
    } else {
        builder_->CreateRetVoid();
    }
}

// === Expression Generation ===

llvm::Value* CodeGenerator::gen_expr(ast::Expr* expr) {
    if (!expr) return nullptr;
    
    if (auto* lit = dynamic_cast<ast::IntLiteral*>(expr)) {
        return builder_->getInt64(lit->value);
    }
    if (auto* lit = dynamic_cast<ast::FloatLiteral*>(expr)) {
        return llvm::ConstantFP::get(builder_->getDoubleTy(), lit->value);
    }
    if (auto* lit = dynamic_cast<ast::StringLiteral*>(expr)) {
        return builder_->CreateGlobalString(lit->value, "str", 0, module_.get());
    }
    if (auto* lit = dynamic_cast<ast::BoolLiteral*>(expr)) {
        return builder_->getInt1(lit->value);
    }
    if (dynamic_cast<ast::NoneLiteral*>(expr)) {
        return llvm::Constant::getNullValue(builder_->getPtrTy());
    }
    if (auto* id = dynamic_cast<ast::Identifier*>(expr)) {
        return gen_identifier(id);
    }
    if (auto* bin = dynamic_cast<ast::BinaryExpr*>(expr)) {
        return gen_binary(bin);
    }
    if (auto* un = dynamic_cast<ast::UnaryExpr*>(expr)) {
        return gen_unary(un);
    }
    if (auto* call = dynamic_cast<ast::CallExpr*>(expr)) {
        return gen_call(call);
    }
    if (auto* assign = dynamic_cast<ast::AssignExpr*>(expr)) {
        return gen_assign(assign);
    }
    if (auto* range = dynamic_cast<ast::RangeExpr*>(expr)) {
        // Return end value for simple for-loop support
        return gen_expr(range->end.get());
    }
    
    return nullptr;
}

llvm::Value* CodeGenerator::gen_identifier(ast::Identifier* id) {
    auto it = named_values_.find(id->name);
    if (it != named_values_.end()) {
        // Load from alloca
        auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(it->second);
        if (alloca) {
            return builder_->CreateLoad(alloca->getAllocatedType(), alloca, id->name);
        }
    }
    
    // Check if it's a function
    auto fn_it = functions_.find(id->name);
    if (fn_it != functions_.end()) {
        return fn_it->second;
    }
    
    error("Unknown identifier: " + id->name, id->location);
    return nullptr;
}

llvm::Value* CodeGenerator::gen_binary(ast::BinaryExpr* binary) {
    auto* left = gen_expr(binary->left.get());
    auto* right = gen_expr(binary->right.get());
    
    if (!left || !right) return nullptr;
    
    bool is_float = left->getType()->isFloatingPointTy();
    
    switch (binary->op) {
        case ast::BinaryOp::Add:
            return is_float ? builder_->CreateFAdd(left, right, "addtmp")
                           : builder_->CreateAdd(left, right, "addtmp");
        case ast::BinaryOp::Sub:
            return is_float ? builder_->CreateFSub(left, right, "subtmp")
                           : builder_->CreateSub(left, right, "subtmp");
        case ast::BinaryOp::Mul:
            return is_float ? builder_->CreateFMul(left, right, "multmp")
                           : builder_->CreateMul(left, right, "multmp");
        case ast::BinaryOp::Div:
            return is_float ? builder_->CreateFDiv(left, right, "divtmp")
                           : builder_->CreateSDiv(left, right, "divtmp");
        case ast::BinaryOp::Mod:
            return builder_->CreateSRem(left, right, "modtmp");
        case ast::BinaryOp::Eq:
            return is_float ? builder_->CreateFCmpOEQ(left, right, "eqtmp")
                           : builder_->CreateICmpEQ(left, right, "eqtmp");
        case ast::BinaryOp::Ne:
            return is_float ? builder_->CreateFCmpONE(left, right, "netmp")
                           : builder_->CreateICmpNE(left, right, "netmp");
        case ast::BinaryOp::Lt:
            return is_float ? builder_->CreateFCmpOLT(left, right, "lttmp")
                           : builder_->CreateICmpSLT(left, right, "lttmp");
        case ast::BinaryOp::Le:
            return is_float ? builder_->CreateFCmpOLE(left, right, "letmp")
                           : builder_->CreateICmpSLE(left, right, "letmp");
        case ast::BinaryOp::Gt:
            return is_float ? builder_->CreateFCmpOGT(left, right, "gttmp")
                           : builder_->CreateICmpSGT(left, right, "gttmp");
        case ast::BinaryOp::Ge:
            return is_float ? builder_->CreateFCmpOGE(left, right, "getmp")
                           : builder_->CreateICmpSGE(left, right, "getmp");
        case ast::BinaryOp::And:
            return builder_->CreateAnd(left, right, "andtmp");
        case ast::BinaryOp::Or:
            return builder_->CreateOr(left, right, "ortmp");
        case ast::BinaryOp::BitAnd:
            return builder_->CreateAnd(left, right, "bandtmp");
        case ast::BinaryOp::BitOr:
            return builder_->CreateOr(left, right, "bortmp");
        case ast::BinaryOp::BitXor:
            return builder_->CreateXor(left, right, "xortmp");
        case ast::BinaryOp::Shl:
            return builder_->CreateShl(left, right, "shltmp");
        case ast::BinaryOp::Shr:
            return builder_->CreateAShr(left, right, "shrtmp");
        case ast::BinaryOp::Pow:
            // Use intrinsic for power
            return builder_->CreateCall(
                llvm::Intrinsic::getOrInsertDeclaration(module_.get(), llvm::Intrinsic::powi, {builder_->getDoubleTy(), builder_->getInt32Ty()}),
                {left, builder_->CreateTrunc(right, builder_->getInt32Ty())},
                "powtmp"
            );
        default:
            return nullptr;
    }
}

llvm::Value* CodeGenerator::gen_unary(ast::UnaryExpr* unary) {
    auto* operand = gen_expr(unary->operand.get());
    if (!operand) return nullptr;
    
    switch (unary->op) {
        case ast::UnaryOp::Neg:
            if (operand->getType()->isFloatingPointTy()) {
                return builder_->CreateFNeg(operand, "negtmp");
            }
            return builder_->CreateNeg(operand, "negtmp");
        case ast::UnaryOp::Not:
            return builder_->CreateNot(operand, "nottmp");
        case ast::UnaryOp::BitNot:
            return builder_->CreateNot(operand, "bnottmp");
    }
    return nullptr;
}

llvm::Value* CodeGenerator::gen_call(ast::CallExpr* call) {
    // Get callee
    llvm::Function* callee = nullptr;
    std::string fn_name;
    
    if (auto* id = dynamic_cast<ast::Identifier*>(call->callee.get())) {
        fn_name = id->name;
        auto it = functions_.find(fn_name);
        if (it != functions_.end()) {
            callee = it->second;
        }
    }
    
    if (!callee) {
        error("Unknown function: " + fn_name, call->location);
        return nullptr;
    }
    
    // Generate arguments
    std::vector<llvm::Value*> args;
    for (auto& arg : call->arguments) {
        auto* val = gen_expr(arg.get());
        if (val) args.push_back(val);
    }
    
    return builder_->CreateCall(callee, args, 
        callee->getReturnType()->isVoidTy() ? "" : "calltmp");
}

llvm::Value* CodeGenerator::gen_index(ast::IndexExpr* /*index*/) {
    // TODO: Array/list indexing
    return nullptr;
}

llvm::Value* CodeGenerator::gen_member(ast::MemberExpr* /*member*/) {
    // TODO: Struct member access
    return nullptr;
}

llvm::Value* CodeGenerator::gen_assign(ast::AssignExpr* assign) {
    auto* val = gen_expr(assign->value.get());
    if (!val) return nullptr;
    
    if (auto* id = dynamic_cast<ast::Identifier*>(assign->target.get())) {
        auto it = named_values_.find(id->name);
        if (it != named_values_.end()) {
            builder_->CreateStore(val, it->second);
            return val;
        }
    }
    
    return nullptr;
}

// === Builtins ===

void CodeGenerator::declare_builtins() {
    // Declare printf for print function
    auto* printf_type = llvm::FunctionType::get(
        builder_->getInt32Ty(),
        {builder_->getPtrTy()},
        true  // vararg
    );
    auto* printf_fn = llvm::Function::Create(
        printf_type,
        llvm::Function::ExternalLinkage,
        "printf",
        module_.get()
    );
    functions_["printf"] = printf_fn;
    
    // Create Axiom print wrapper
    auto* print_type = llvm::FunctionType::get(
        builder_->getVoidTy(),
        {builder_->getPtrTy()},
        false
    );
    auto* print_fn = llvm::Function::Create(
        print_type,
        llvm::Function::ExternalLinkage,
        "print",
        module_.get()
    );
    
    // Implement print
    auto* entry = llvm::BasicBlock::Create(*context_, "entry", print_fn);
    builder_->SetInsertPoint(entry);
    auto* format = builder_->CreateGlobalString("%s\n", "fmt", 0, module_.get());
    builder_->CreateCall(printf_fn, {format, print_fn->getArg(0)});
    builder_->CreateRetVoid();
    
    functions_["print"] = print_fn;
}

llvm::Function* CodeGenerator::get_printf() {
    return functions_["printf"];
}

// === Helpers ===

void CodeGenerator::error(const std::string& msg, const SourceLocation& loc) {
    errors_.emplace_back(msg, loc);
}

llvm::AllocaInst* CodeGenerator::create_alloca(llvm::Function* fn,
                                                const std::string& name,
                                                llvm::Type* type) {
    llvm::IRBuilder<> tmp_builder(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmp_builder.CreateAlloca(type, nullptr, name);
}

} // namespace codegen
} // namespace axiom
