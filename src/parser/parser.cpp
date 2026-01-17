/**
 * @file parser.cpp
 * @brief Recursive descent parser implementation for Axiom
 */

#include "parser.hpp"
#include <unordered_map>
#include <stdexcept>

namespace axiom {

// Operator precedence table (higher = tighter binding)
namespace {
    enum Precedence {
        PREC_NONE = 0,
        PREC_ASSIGNMENT,   // =
        PREC_OR,           // or
        PREC_AND,          // and
        PREC_EQUALITY,     // == !=
        PREC_COMPARISON,   // < > <= >=
        PREC_BIT_OR,       // |
        PREC_BIT_XOR,      // ^
        PREC_BIT_AND,      // &
        PREC_SHIFT,        // << >>
        PREC_RANGE,        // ..
        PREC_TERM,         // + -
        PREC_FACTOR,       // * / %
        PREC_POWER,        // **
        PREC_UNARY,        // ! - ~
        PREC_CALL,         // . () []
        PREC_PRIMARY,
    };
    
    std::unordered_map<TokenType, int> PRECEDENCE = {
        {TokenType::ASSIGN, PREC_ASSIGNMENT},
        {TokenType::PLUS_ASSIGN, PREC_ASSIGNMENT},
        {TokenType::MINUS_ASSIGN, PREC_ASSIGNMENT},
        {TokenType::STAR_ASSIGN, PREC_ASSIGNMENT},
        {TokenType::SLASH_ASSIGN, PREC_ASSIGNMENT},
        {TokenType::OR, PREC_OR},
        {TokenType::AND, PREC_AND},
        {TokenType::EQ, PREC_EQUALITY},
        {TokenType::NE, PREC_EQUALITY},
        {TokenType::LT, PREC_COMPARISON},
        {TokenType::LE, PREC_COMPARISON},
        {TokenType::GT, PREC_COMPARISON},
        {TokenType::GE, PREC_COMPARISON},
        {TokenType::PIPE, PREC_BIT_OR},
        {TokenType::CARET, PREC_BIT_XOR},
        {TokenType::AMPERSAND, PREC_BIT_AND},
        {TokenType::SHL, PREC_SHIFT},
        {TokenType::SHR, PREC_SHIFT},
        {TokenType::DOUBLE_DOT, PREC_RANGE},
        {TokenType::PLUS, PREC_TERM},
        {TokenType::MINUS, PREC_TERM},
        {TokenType::STAR, PREC_FACTOR},
        {TokenType::SLASH, PREC_FACTOR},
        {TokenType::PERCENT, PREC_FACTOR},
        {TokenType::AT, PREC_FACTOR},  // @ for matmul
        {TokenType::POWER, PREC_POWER},
        {TokenType::LPAREN, PREC_CALL},
        {TokenType::LBRACKET, PREC_CALL},
        {TokenType::DOT, PREC_CALL},
    };
}

Parser::Parser(Lexer& lexer) : lexer_(lexer) {
    // Prime the parser with first token
    current_ = lexer_.next_token();
}

// === Token Management ===

Token Parser::current_token() {
    return current_;
}

Token Parser::peek_token() {
    return lexer_.peek_token();
}

Token Parser::advance() {
    previous_ = current_;
    
    // Skip over NEWLINE and INDENT/DEDENT tokens when not significant
    do {
        current_ = lexer_.next_token();
    } while (current_.type == TokenType::NEWLINE && !expect_indent_);
    
    return previous_;
}

bool Parser::check(TokenType type) {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

bool Parser::match_any(std::initializer_list<TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::expect(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    error(message + ", got " + std::string(token_type_name(current_.type)));
    return current_;
}

// === Error Handling ===

void Parser::error(const std::string& message) {
    if (panic_mode_) return;  // Suppress cascading errors
    panic_mode_ = true;
    errors_.emplace_back(message, current_.location);
}

void Parser::synchronize() {
    panic_mode_ = false;
    
    while (!check(TokenType::EOF_TOKEN)) {
        // Synchronize at statement boundaries
        if (previous_.type == TokenType::NEWLINE) return;
        
        switch (current_.type) {
            case TokenType::FN:
            case TokenType::LET:
            case TokenType::VAR:
            case TokenType::CONST:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::FOR:
            case TokenType::RETURN:
            case TokenType::STRUCT:
            case TokenType::CLASS:
            case TokenType::TRAIT:
            case TokenType::IMPL:
                return;
            default:
                break;
        }
        advance();
    }
}

// === Main Parse Entry ===

ast::Program Parser::parse() {
    ast::Program program;
    
    while (!check(TokenType::EOF_TOKEN)) {
        try {
            // Skip leading newlines
            while (match(TokenType::NEWLINE)) {}
            
            if (check(TokenType::EOF_TOKEN)) break;
            
            auto decl = parse_declaration();
            if (decl) {
                program.declarations.push_back(std::move(decl));
            }
        } catch (...) {
            synchronize();
        }
    }
    
    return program;
}

// === Declaration Parsing ===

ast::DeclPtr Parser::parse_declaration() {
    // Check for pub modifier
    bool is_public = match(TokenType::PUB);
    
    ast::DeclPtr decl;
    
    if (check(TokenType::FN) || check(TokenType::ASYNC)) {
        decl = parse_function();
    } else if (check(TokenType::STRUCT)) {
        decl = parse_struct();
    } else if (check(TokenType::CLASS)) {
        decl = parse_class();
    } else if (check(TokenType::TRAIT)) {
        decl = parse_trait();
    } else if (check(TokenType::IMPL)) {
        decl = parse_impl();
    } else if (check(TokenType::ENUM)) {
        decl = parse_enum();
    } else if (check(TokenType::TYPE)) {
        decl = parse_type_alias();
    } else if (check(TokenType::IMPORT) || check(TokenType::FROM)) {
        decl = parse_import();
    } else {
        // Fall back to parsing as a statement
        auto stmt = parse_statement();
        // Wrap in a declaration if needed - for now just ignore top-level statements
        return nullptr;
    }
    
    if (decl) {
        decl->is_public = is_public;
    }
    
    return decl;
}

ast::DeclPtr Parser::parse_function() {
    bool is_async = match(TokenType::ASYNC);
    expect(TokenType::FN, "Expected 'fn'");
    
    Token name_token = expect(TokenType::IDENTIFIER, "Expected function name");
    auto fn = std::make_unique<ast::FnDecl>(std::string(name_token.lexeme));
    fn->is_async = is_async;
    fn->location = name_token.location;
    
    // Optional type parameters [T, U]
    if (match(TokenType::LBRACKET)) {
        fn->type_params = parse_type_params();
        expect(TokenType::RBRACKET, "Expected ']' after type parameters");
    }
    
    // Parameters
    expect(TokenType::LPAREN, "Expected '(' after function name");
    if (!check(TokenType::RPAREN)) {
        fn->params = parse_param_list();
    }
    expect(TokenType::RPAREN, "Expected ')' after parameters");
    
    // Return type
    if (match(TokenType::ARROW)) {
        fn->return_type = parse_type();
    }
    
    // Function body
    expect(TokenType::COLON, "Expected ':' before function body");
    fn->body = parse_block();
    
    return fn;
}

ast::DeclPtr Parser::parse_struct() {
    expect(TokenType::STRUCT, "Expected 'struct'");
    
    Token name_token = expect(TokenType::IDENTIFIER, "Expected struct name");
    auto st = std::make_unique<ast::StructDecl>(std::string(name_token.lexeme));
    st->location = name_token.location;
    
    // Optional type parameters
    if (match(TokenType::LBRACKET)) {
        st->type_params = parse_type_params();
        expect(TokenType::RBRACKET, "Expected ']'");
    }
    
    expect(TokenType::COLON, "Expected ':' before struct body");
    
    // Parse fields and methods
    // Skip NEWLINE and look for INDENT
    while (match(TokenType::NEWLINE)) {}
    
    while (!check(TokenType::EOF_TOKEN) && !check(TokenType::DEDENT)) {
        while (match(TokenType::NEWLINE)) {}
        
        if (check(TokenType::DEDENT) || check(TokenType::EOF_TOKEN)) break;
        
        bool field_public = match(TokenType::PUB);
        
        if (check(TokenType::FN)) {
            // Method
            auto method = parse_function();
            st->methods.push_back(
                std::unique_ptr<ast::FnDecl>(static_cast<ast::FnDecl*>(method.release()))
            );
        } else if (check(TokenType::IDENTIFIER)) {
            // Field
            Token field_name = advance();
            expect(TokenType::COLON, "Expected ':' after field name");
            auto field_type = parse_type();
            
            ast::StructField field;
            field.name = std::string(field_name.lexeme);
            field.type = std::move(field_type);
            field.is_public = field_public;
            
            // Optional default value
            if (match(TokenType::ASSIGN)) {
                field.default_value = parse_expression();
            }
            
            st->fields.push_back(std::move(field));
        } else {
            error("Expected field or method in struct");
            synchronize();
        }
        
        while (match(TokenType::NEWLINE)) {}
    }
    
    match(TokenType::DEDENT);
    
    return st;
}

ast::DeclPtr Parser::parse_class() {
    expect(TokenType::CLASS, "Expected 'class'");
    
    Token name_token = expect(TokenType::IDENTIFIER, "Expected class name");
    auto cls = std::make_unique<ast::ClassDecl>(std::string(name_token.lexeme));
    cls->location = name_token.location;
    
    // Optional base class
    if (match(TokenType::LPAREN)) {
        Token base = expect(TokenType::IDENTIFIER, "Expected base class name");
        cls->base_class = std::string(base.lexeme);
        expect(TokenType::RPAREN, "Expected ')' after base class");
    }
    
    expect(TokenType::COLON, "Expected ':' before class body");
    
    // Parse fields and methods (similar to struct)
    while (match(TokenType::NEWLINE)) {}
    
    while (!check(TokenType::EOF_TOKEN) && !check(TokenType::DEDENT)) {
        while (match(TokenType::NEWLINE)) {}
        
        if (check(TokenType::DEDENT) || check(TokenType::EOF_TOKEN)) break;
        
        bool member_public = match(TokenType::PUB);
        
        if (check(TokenType::FN)) {
            auto method = parse_function();
            cls->methods.push_back(
                std::unique_ptr<ast::FnDecl>(static_cast<ast::FnDecl*>(method.release()))
            );
        } else if (check(TokenType::IDENTIFIER)) {
            Token field_name = advance();
            expect(TokenType::COLON, "Expected ':' after field name");
            auto field_type = parse_type();
            
            ast::StructField field;
            field.name = std::string(field_name.lexeme);
            field.type = std::move(field_type);
            field.is_public = member_public;
            
            if (match(TokenType::ASSIGN)) {
                field.default_value = parse_expression();
            }
            
            cls->fields.push_back(std::move(field));
        } else {
            error("Expected field or method in class");
            synchronize();
        }
        
        while (match(TokenType::NEWLINE)) {}
    }
    
    match(TokenType::DEDENT);
    
    return cls;
}

ast::DeclPtr Parser::parse_trait() {
    expect(TokenType::TRAIT, "Expected 'trait'");
    
    Token name_token = expect(TokenType::IDENTIFIER, "Expected trait name");
    auto trait = std::make_unique<ast::TraitDecl>(std::string(name_token.lexeme));
    trait->location = name_token.location;
    
    // Optional type parameters
    if (match(TokenType::LBRACKET)) {
        trait->type_params = parse_type_params();
        expect(TokenType::RBRACKET, "Expected ']'");
    }
    
    expect(TokenType::COLON, "Expected ':' before trait body");
    
    // Parse method signatures
    while (match(TokenType::NEWLINE)) {}
    
    while (!check(TokenType::EOF_TOKEN) && !check(TokenType::DEDENT)) {
        while (match(TokenType::NEWLINE)) {}
        
        if (check(TokenType::DEDENT) || check(TokenType::EOF_TOKEN)) break;
        
        if (check(TokenType::FN)) {
            auto method = parse_function();
            trait->methods.push_back(
                std::unique_ptr<ast::FnDecl>(static_cast<ast::FnDecl*>(method.release()))
            );
        } else {
            error("Expected method in trait");
            synchronize();
        }
        
        while (match(TokenType::NEWLINE)) {}
    }
    
    match(TokenType::DEDENT);
    
    return trait;
}

ast::DeclPtr Parser::parse_impl() {
    expect(TokenType::IMPL, "Expected 'impl'");
    
    Token first = expect(TokenType::IDENTIFIER, "Expected type or trait name");
    auto impl = std::make_unique<ast::ImplDecl>(std::string(first.lexeme));
    impl->location = first.location;
    
    // Check if this is `impl Trait for Type`
    if (match(TokenType::FOR)) {
        impl->trait_name = std::string(first.lexeme);
        Token type_name = expect(TokenType::IDENTIFIER, "Expected type name");
        impl->type_name = std::string(type_name.lexeme);
    }
    
    expect(TokenType::COLON, "Expected ':' before impl body");
    
    // Parse methods
    while (match(TokenType::NEWLINE)) {}
    
    while (!check(TokenType::EOF_TOKEN) && !check(TokenType::DEDENT)) {
        while (match(TokenType::NEWLINE)) {}
        
        if (check(TokenType::DEDENT) || check(TokenType::EOF_TOKEN)) break;
        
        if (check(TokenType::FN)) {
            auto method = parse_function();
            impl->methods.push_back(
                std::unique_ptr<ast::FnDecl>(static_cast<ast::FnDecl*>(method.release()))
            );
        } else {
            error("Expected method in impl block");
            synchronize();
        }
        
        while (match(TokenType::NEWLINE)) {}
    }
    
    match(TokenType::DEDENT);
    
    return impl;
}

ast::DeclPtr Parser::parse_enum() {
    expect(TokenType::ENUM, "Expected 'enum'");
    
    Token name_token = expect(TokenType::IDENTIFIER, "Expected enum name");
    auto en = std::make_unique<ast::EnumDecl>(std::string(name_token.lexeme));
    en->location = name_token.location;
    
    // Optional type parameters
    if (match(TokenType::LBRACKET)) {
        en->type_params = parse_type_params();
        expect(TokenType::RBRACKET, "Expected ']'");
    }
    
    expect(TokenType::COLON, "Expected ':' before enum body");
    
    // Parse variants
    while (match(TokenType::NEWLINE)) {}
    
    while (!check(TokenType::EOF_TOKEN) && !check(TokenType::DEDENT)) {
        while (match(TokenType::NEWLINE)) {}
        
        if (check(TokenType::DEDENT) || check(TokenType::EOF_TOKEN)) break;
        
        Token variant_name = expect(TokenType::IDENTIFIER, "Expected variant name");
        ast::EnumVariant variant;
        variant.name = std::string(variant_name.lexeme);
        
        // Optional tuple fields
        if (match(TokenType::LPAREN)) {
            while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
                variant.fields.push_back(parse_type());
                if (!match(TokenType::COMMA)) break;
            }
            expect(TokenType::RPAREN, "Expected ')' after variant fields");
        }
        
        en->variants.push_back(std::move(variant));
        
        while (match(TokenType::NEWLINE)) {}
    }
    
    match(TokenType::DEDENT);
    
    return en;
}

ast::DeclPtr Parser::parse_type_alias() {
    expect(TokenType::TYPE, "Expected 'type'");
    
    Token name_token = expect(TokenType::IDENTIFIER, "Expected type name");
    expect(TokenType::ASSIGN, "Expected '=' after type name");
    auto aliased = parse_type();
    
    return std::make_unique<ast::TypeAliasDecl>(
        std::string(name_token.lexeme), std::move(aliased)
    );
}

ast::DeclPtr Parser::parse_import() {
    if (match(TokenType::IMPORT)) {
        Token module = expect(TokenType::IDENTIFIER, "Expected module name");
        auto import = std::make_unique<ast::ImportDecl>(std::string(module.lexeme));
        
        // Handle dotted path: import std.collections
        while (match(TokenType::DOT)) {
            Token next = expect(TokenType::IDENTIFIER, "Expected module name");
            import->module_path += "." + std::string(next.lexeme);
        }
        
        // Optional alias: import x as y
        if (match(TokenType::AS)) {
            Token alias = expect(TokenType::IDENTIFIER, "Expected alias name");
            import->alias = std::string(alias.lexeme);
        }
        
        return import;
    } else {
        // from x import y, z
        expect(TokenType::FROM, "Expected 'from'");
        Token module = expect(TokenType::IDENTIFIER, "Expected module name");
        auto import = std::make_unique<ast::ImportDecl>(std::string(module.lexeme));
        
        while (match(TokenType::DOT)) {
            Token next = expect(TokenType::IDENTIFIER, "Expected module name");
            import->module_path += "." + std::string(next.lexeme);
        }
        
        expect(TokenType::IMPORT, "Expected 'import'");
        
        if (match(TokenType::STAR)) {
            import->import_all = true;
        } else {
            do {
                Token sym = expect(TokenType::IDENTIFIER, "Expected symbol name");
                import->symbols.push_back(std::string(sym.lexeme));
            } while (match(TokenType::COMMA));
        }
        
        return import;
    }
}

// === Statement Parsing ===

ast::StmtPtr Parser::parse_statement() {
    if (check(TokenType::IF)) return parse_if_statement();
    if (check(TokenType::WHILE)) return parse_while_statement();
    if (check(TokenType::FOR)) return parse_for_statement();
    if (check(TokenType::MATCH)) return parse_match_statement();
    if (check(TokenType::RETURN)) return parse_return_statement();
    if (check(TokenType::BREAK)) { advance(); return std::make_unique<ast::BreakStmt>(); }
    if (check(TokenType::CONTINUE)) { advance(); return std::make_unique<ast::ContinueStmt>(); }
    if (check(TokenType::LET) || check(TokenType::VAR) || check(TokenType::CONST)) {
        return parse_var_decl_statement();
    }
    if (check(TokenType::YIELD)) {
        advance();
        auto expr = parse_expression();
        return std::make_unique<ast::YieldStmt>(std::move(expr));
    }
    
    // Expression statement
    auto expr = parse_expression();
    return std::make_unique<ast::ExprStmt>(std::move(expr));
}

ast::StmtPtr Parser::parse_if_statement() {
    expect(TokenType::IF, "Expected 'if'");
    auto condition = parse_expression();
    expect(TokenType::COLON, "Expected ':' after if condition");
    auto then_block = parse_block();
    
    auto if_stmt = std::make_unique<ast::IfStmt>(std::move(condition), std::move(then_block));
    
    // Parse elif chains
    while (match(TokenType::ELIF)) {
        auto elif_cond = parse_expression();
        expect(TokenType::COLON, "Expected ':' after elif condition");
        auto elif_block = parse_block();
        if_stmt->elif_blocks.emplace_back(std::move(elif_cond), std::move(elif_block));
    }
    
    // Parse else
    if (match(TokenType::ELSE)) {
        expect(TokenType::COLON, "Expected ':' after else");
        if_stmt->else_block = parse_block();
    }
    
    return if_stmt;
}

ast::StmtPtr Parser::parse_while_statement() {
    expect(TokenType::WHILE, "Expected 'while'");
    auto condition = parse_expression();
    expect(TokenType::COLON, "Expected ':' after while condition");
    auto body = parse_block();
    
    return std::make_unique<ast::WhileStmt>(std::move(condition), std::move(body));
}

ast::StmtPtr Parser::parse_for_statement() {
    expect(TokenType::FOR, "Expected 'for'");
    Token var = expect(TokenType::IDENTIFIER, "Expected loop variable");
    expect(TokenType::IN, "Expected 'in'");
    auto iterable = parse_expression();
    expect(TokenType::COLON, "Expected ':' after for header");
    auto body = parse_block();
    
    return std::make_unique<ast::ForStmt>(
        std::string(var.lexeme), std::move(iterable), std::move(body)
    );
}

ast::StmtPtr Parser::parse_match_statement() {
    expect(TokenType::MATCH, "Expected 'match'");
    auto value = parse_expression();
    expect(TokenType::COLON, "Expected ':' after match value");
    
    std::vector<ast::MatchArm> arms;
    
    while (match(TokenType::NEWLINE)) {}
    
    while (check(TokenType::CASE)) {
        advance();  // consume 'case'
        auto pattern = parse_expression();
        
        std::optional<ast::ExprPtr> guard;
        if (match(TokenType::IF)) {
            guard = parse_expression();
        }
        
        expect(TokenType::COLON, "Expected ':' after case pattern");
        auto body = parse_block();
        
        ast::MatchArm arm;
        arm.pattern = std::move(pattern);
        arm.guard = std::move(guard);
        arm.body = std::move(body);
        arms.push_back(std::move(arm));
        
        while (match(TokenType::NEWLINE)) {}
    }
    
    match(TokenType::DEDENT);
    
    return std::make_unique<ast::MatchStmt>(std::move(value), std::move(arms));
}

ast::StmtPtr Parser::parse_return_statement() {
    expect(TokenType::RETURN, "Expected 'return'");
    
    std::optional<ast::ExprPtr> value;
    if (!check(TokenType::NEWLINE) && !check(TokenType::EOF_TOKEN)) {
        value = parse_expression();
    }
    
    return std::make_unique<ast::ReturnStmt>(std::move(value));
}

ast::StmtPtr Parser::parse_var_decl_statement() {
    bool is_const = match(TokenType::CONST);
    bool is_mutable = !is_const && match(TokenType::VAR);
    if (!is_const && !is_mutable) {
        expect(TokenType::LET, "Expected 'let', 'var', or 'const'");
    }
    
    Token name = expect(TokenType::IDENTIFIER, "Expected variable name");
    auto var_decl = std::make_unique<ast::VarDeclStmt>(
        std::string(name.lexeme), is_mutable, is_const
    );
    var_decl->location = name.location;
    
    // Optional type annotation
    if (match(TokenType::COLON)) {
        var_decl->type = parse_type();
    }
    
    // Optional initializer
    if (match(TokenType::ASSIGN)) {
        var_decl->initializer = parse_expression();
    }
    
    return var_decl;
}

ast::BlockPtr Parser::parse_block() {
    auto block = std::make_unique<ast::Block>();
    
    // Skip leading newlines
    while (match(TokenType::NEWLINE)) {}
    
    // Parse statements until DEDENT or EOF
    while (!check(TokenType::DEDENT) && !check(TokenType::EOF_TOKEN)) {
        // Skip blank lines
        while (match(TokenType::NEWLINE)) {}
        
        if (check(TokenType::DEDENT) || check(TokenType::EOF_TOKEN)) break;
        
        // Check for nested declarations or statements
        if (check(TokenType::FN) || check(TokenType::STRUCT) || 
            check(TokenType::CLASS) || check(TokenType::TRAIT)) {
            // Nested declaration - skip for now (methods handled in struct/class)
            error("Unexpected declaration in block");
            synchronize();
            continue;
        }
        
        auto stmt = parse_statement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
        
        // Consume newlines between statements
        while (match(TokenType::NEWLINE)) {}
    }
    
    match(TokenType::DEDENT);
    
    return block;
}

// === Expression Parsing (Pratt Parser) ===

ast::ExprPtr Parser::parse_expression() {
    return parse_expression_with_precedence(PREC_ASSIGNMENT);
}

ast::ExprPtr Parser::parse_expression_with_precedence(int min_precedence) {
    auto left = parse_prefix();
    
    while (get_precedence(current_.type) >= min_precedence) {
        left = parse_infix(std::move(left), get_precedence(current_.type));
    }
    
    return left;
}

ast::ExprPtr Parser::parse_prefix() {
    // Unary operators
    if (match(TokenType::MINUS)) {
        auto operand = parse_expression_with_precedence(PREC_UNARY);
        return std::make_unique<ast::UnaryExpr>(ast::UnaryOp::Neg, std::move(operand));
    }
    if (match(TokenType::NOT)) {
        auto operand = parse_expression_with_precedence(PREC_UNARY);
        return std::make_unique<ast::UnaryExpr>(ast::UnaryOp::Not, std::move(operand));
    }
    if (match(TokenType::TILDE)) {
        auto operand = parse_expression_with_precedence(PREC_UNARY);
        return std::make_unique<ast::UnaryExpr>(ast::UnaryOp::BitNot, std::move(operand));
    }
    
    // Await expression
    if (match(TokenType::AWAIT)) {
        auto operand = parse_expression_with_precedence(PREC_UNARY);
        return std::make_unique<ast::AwaitExpr>(std::move(operand));
    }
    
    return parse_postfix(parse_primary());
}

ast::ExprPtr Parser::parse_infix(ast::ExprPtr left, int precedence) {
    TokenType op_type = current_.type;
    advance();
    
    // Right-associative for ** (power)
    int next_prec = (op_type == TokenType::POWER) ? precedence : precedence + 1;
    auto right = parse_expression_with_precedence(next_prec);
    
    // Check for assignment operators
    if (op_type == TokenType::ASSIGN) {
        return std::make_unique<ast::AssignExpr>(std::move(left), std::move(right));
    }
    if (op_type == TokenType::PLUS_ASSIGN) {
        return std::make_unique<ast::AssignExpr>(std::move(left), std::move(right), ast::BinaryOp::Add);
    }
    if (op_type == TokenType::MINUS_ASSIGN) {
        return std::make_unique<ast::AssignExpr>(std::move(left), std::move(right), ast::BinaryOp::Sub);
    }
    if (op_type == TokenType::STAR_ASSIGN) {
        return std::make_unique<ast::AssignExpr>(std::move(left), std::move(right), ast::BinaryOp::Mul);
    }
    if (op_type == TokenType::SLASH_ASSIGN) {
        return std::make_unique<ast::AssignExpr>(std::move(left), std::move(right), ast::BinaryOp::Div);
    }
    
    // Range operator
    if (op_type == TokenType::DOUBLE_DOT) {
        return std::make_unique<ast::RangeExpr>(std::move(left), std::move(right), false);
    }
    
    // Binary operators
    return std::make_unique<ast::BinaryExpr>(
        token_to_binary_op(op_type), std::move(left), std::move(right)
    );
}

ast::ExprPtr Parser::parse_postfix(ast::ExprPtr operand) {
    while (true) {
        if (match(TokenType::LPAREN)) {
            // Function call
            std::vector<ast::ExprPtr> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parse_expression());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN, "Expected ')' after arguments");
            operand = std::make_unique<ast::CallExpr>(std::move(operand), std::move(args));
        } else if (match(TokenType::LBRACKET)) {
            // Index or slice
            auto index = parse_expression();
            expect(TokenType::RBRACKET, "Expected ']' after index");
            operand = std::make_unique<ast::IndexExpr>(std::move(operand), std::move(index));
        } else if (match(TokenType::DOT)) {
            // Member access
            Token member = expect(TokenType::IDENTIFIER, "Expected member name");
            operand = std::make_unique<ast::MemberExpr>(
                std::move(operand), std::string(member.lexeme)
            );
        } else {
            break;
        }
    }
    return operand;
}

ast::ExprPtr Parser::parse_primary() {
    // Literals
    if (check(TokenType::INTEGER)) {
        Token tok = advance();
        return std::make_unique<ast::IntLiteral>(tok.int_value);
    }
    if (check(TokenType::FLOAT)) {
        Token tok = advance();
        return std::make_unique<ast::FloatLiteral>(tok.float_value);
    }
    if (check(TokenType::STRING)) {
        Token tok = advance();
        return std::make_unique<ast::StringLiteral>(std::string(tok.lexeme));
    }
    if (match(TokenType::TRUE)) {
        return std::make_unique<ast::BoolLiteral>(true);
    }
    if (match(TokenType::FALSE)) {
        return std::make_unique<ast::BoolLiteral>(false);
    }
    if (match(TokenType::NONE)) {
        return std::make_unique<ast::NoneLiteral>();
    }
    
    // Identifier
    if (check(TokenType::IDENTIFIER)) {
        Token tok = advance();
        return std::make_unique<ast::Identifier>(std::string(tok.lexeme));
    }
    
    // Grouped expression or tuple
    if (check(TokenType::LPAREN)) {
        return parse_tuple_or_grouped();
    }
    
    // List or list comprehension
    if (check(TokenType::LBRACKET)) {
        return parse_list_or_comprehension();
    }
    
    // Dict
    if (check(TokenType::LBRACE)) {
        return parse_dict_or_set();
    }
    
    // Lambda
    if (check(TokenType::PIPE)) {
        return parse_lambda();
    }
    
    error("Expected expression");
    return nullptr;
}

ast::ExprPtr Parser::parse_tuple_or_grouped() {
    expect(TokenType::LPAREN, "Expected '('");
    
    if (match(TokenType::RPAREN)) {
        // Empty tuple
        return std::make_unique<ast::TupleExpr>(std::vector<ast::ExprPtr>{});
    }
    
    auto first = parse_expression();
    
    if (match(TokenType::COMMA)) {
        // Tuple
        std::vector<ast::ExprPtr> elements;
        elements.push_back(std::move(first));
        
        if (!check(TokenType::RPAREN)) {
            do {
                elements.push_back(parse_expression());
            } while (match(TokenType::COMMA));
        }
        
        expect(TokenType::RPAREN, "Expected ')'");
        return std::make_unique<ast::TupleExpr>(std::move(elements));
    }
    
    expect(TokenType::RPAREN, "Expected ')'");
    return first;  // Just a grouped expression
}

ast::ExprPtr Parser::parse_list_or_comprehension() {
    expect(TokenType::LBRACKET, "Expected '['");
    
    if (match(TokenType::RBRACKET)) {
        return std::make_unique<ast::ListExpr>(std::vector<ast::ExprPtr>{});
    }
    
    auto first = parse_expression();
    
    // Check for list comprehension
    if (match(TokenType::FOR)) {
        Token var = expect(TokenType::IDENTIFIER, "Expected variable");
        expect(TokenType::IN, "Expected 'in'");
        auto iterable = parse_expression();
        
        std::optional<ast::ExprPtr> condition;
        if (match(TokenType::IF)) {
            condition = parse_expression();
        }
        
        expect(TokenType::RBRACKET, "Expected ']'");
        
        return std::make_unique<ast::ListCompExpr>(
            std::move(first), std::string(var.lexeme), 
            std::move(iterable), std::move(condition)
        );
    }
    
    // Regular list
    std::vector<ast::ExprPtr> elements;
    elements.push_back(std::move(first));
    
    while (match(TokenType::COMMA)) {
        if (check(TokenType::RBRACKET)) break;
        elements.push_back(parse_expression());
    }
    
    expect(TokenType::RBRACKET, "Expected ']'");
    return std::make_unique<ast::ListExpr>(std::move(elements));
}

ast::ExprPtr Parser::parse_dict_or_set() {
    expect(TokenType::LBRACE, "Expected '{'");
    
    if (match(TokenType::RBRACE)) {
        return std::make_unique<ast::DictExpr>(
            std::vector<std::pair<ast::ExprPtr, ast::ExprPtr>>{}
        );
    }
    
    auto first_key = parse_expression();
    expect(TokenType::COLON, "Expected ':' in dict literal");
    auto first_value = parse_expression();
    
    std::vector<std::pair<ast::ExprPtr, ast::ExprPtr>> entries;
    entries.emplace_back(std::move(first_key), std::move(first_value));
    
    while (match(TokenType::COMMA)) {
        if (check(TokenType::RBRACE)) break;
        auto key = parse_expression();
        expect(TokenType::COLON, "Expected ':'");
        auto value = parse_expression();
        entries.emplace_back(std::move(key), std::move(value));
    }
    
    expect(TokenType::RBRACE, "Expected '}'");
    return std::make_unique<ast::DictExpr>(std::move(entries));
}

ast::ExprPtr Parser::parse_lambda() {
    expect(TokenType::PIPE, "Expected '|'");
    
    std::vector<ast::LambdaExpr::Param> params;
    
    if (!check(TokenType::PIPE)) {
        do {
            Token name = expect(TokenType::IDENTIFIER, "Expected parameter name");
            ast::LambdaExpr::Param param;
            param.name = std::string(name.lexeme);
            
            if (match(TokenType::COLON)) {
                param.type = parse_type();
            }
            
            params.push_back(std::move(param));
        } while (match(TokenType::COMMA));
    }
    
    expect(TokenType::PIPE, "Expected '|' after lambda parameters");
    
    std::optional<ast::TypePtr> return_type;
    if (match(TokenType::ARROW)) {
        return_type = parse_type();
    }
    
    // Lambda body (either block or expression)
    ast::ExprPtr body;
    if (match(TokenType::LBRACE)) {
        // For now, just parse a single expression inside braces
        body = parse_expression();
        expect(TokenType::RBRACE, "Expected '}'");
    } else {
        body = parse_expression();
    }
    
    return std::make_unique<ast::LambdaExpr>(
        std::move(params), std::move(body), std::move(return_type)
    );
}

// === Type Parsing ===

ast::TypePtr Parser::parse_type() {
    if (match(TokenType::AMPERSAND)) {
        bool is_mut = match(TokenType::MUT);
        auto inner = parse_type();
        return std::make_unique<ast::ReferenceType>(std::move(inner), is_mut);
    }
    
    if (match(TokenType::LBRACKET)) {
        // Array type [T; N] or [T]
        auto elem = parse_type();
        std::optional<size_t> size;
        if (match(TokenType::SEMICOLON)) {
            Token size_tok = expect(TokenType::INTEGER, "Expected array size");
            size = static_cast<size_t>(size_tok.int_value);
        }
        expect(TokenType::RBRACKET, "Expected ']'");
        return std::make_unique<ast::ArrayType>(std::move(elem), size);
    }
    
    if (match(TokenType::LPAREN)) {
        // Tuple type (T, U, V)
        std::vector<ast::TypePtr> elements;
        if (!check(TokenType::RPAREN)) {
            do {
                elements.push_back(parse_type());
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::RPAREN, "Expected ')'");
        return std::make_unique<ast::TupleType>(std::move(elements));
    }
    
    if (match(TokenType::FN)) {
        // Function type fn(T, U) -> V
        expect(TokenType::LPAREN, "Expected '(' in function type");
        std::vector<ast::TypePtr> params;
        if (!check(TokenType::RPAREN)) {
            do {
                params.push_back(parse_type());
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::RPAREN, "Expected ')'");
        
        ast::TypePtr ret;
        if (match(TokenType::ARROW)) {
            ret = parse_type();
        }
        
        return std::make_unique<ast::FunctionType>(std::move(params), std::move(ret));
    }
    
    // Simple or generic type
    Token name = expect(TokenType::IDENTIFIER, "Expected type name");
    
    if (match(TokenType::LBRACKET)) {
        // Generic type like List[T]
        std::vector<ast::TypePtr> type_args;
        do {
            type_args.push_back(parse_type());
        } while (match(TokenType::COMMA));
        expect(TokenType::RBRACKET, "Expected ']'");
        
        return std::make_unique<ast::GenericType>(
            std::string(name.lexeme), std::move(type_args)
        );
    }
    
    return std::make_unique<ast::SimpleType>(std::string(name.lexeme));
}

// === Helpers ===

ast::FnParam Parser::parse_function_param() {
    bool is_mut = match(TokenType::MUT);
    Token name = expect(TokenType::IDENTIFIER, "Expected parameter name");
    expect(TokenType::COLON, "Expected ':' after parameter name");
    auto type = parse_type();
    
    ast::FnParam param(std::string(name.lexeme), std::move(type));
    param.is_mutable = is_mut;
    
    if (match(TokenType::ASSIGN)) {
        param.default_value = parse_expression();
    }
    
    return param;
}

std::vector<ast::FnParam> Parser::parse_param_list() {
    std::vector<ast::FnParam> params;
    
    // Handle self parameter
    if (match(TokenType::SELF)) {
        ast::FnParam self_param("self", std::make_unique<ast::SimpleType>("Self"));
        params.push_back(std::move(self_param));
        
        if (!match(TokenType::COMMA)) {
            return params;
        }
    }
    
    do {
        params.push_back(parse_function_param());
    } while (match(TokenType::COMMA));
    
    return params;
}

std::vector<std::string> Parser::parse_type_params() {
    std::vector<std::string> params;
    
    do {
        Token name = expect(TokenType::IDENTIFIER, "Expected type parameter");
        params.push_back(std::string(name.lexeme));
    } while (match(TokenType::COMMA));
    
    return params;
}

int Parser::get_precedence(TokenType type) {
    auto it = PRECEDENCE.find(type);
    if (it != PRECEDENCE.end()) {
        return it->second;
    }
    return PREC_NONE;
}

ast::BinaryOp Parser::token_to_binary_op(TokenType type) {
    switch (type) {
        case TokenType::PLUS:      return ast::BinaryOp::Add;
        case TokenType::MINUS:     return ast::BinaryOp::Sub;
        case TokenType::STAR:      return ast::BinaryOp::Mul;
        case TokenType::SLASH:     return ast::BinaryOp::Div;
        case TokenType::PERCENT:   return ast::BinaryOp::Mod;
        case TokenType::POWER:     return ast::BinaryOp::Pow;
        case TokenType::EQ:        return ast::BinaryOp::Eq;
        case TokenType::NE:        return ast::BinaryOp::Ne;
        case TokenType::LT:        return ast::BinaryOp::Lt;
        case TokenType::LE:        return ast::BinaryOp::Le;
        case TokenType::GT:        return ast::BinaryOp::Gt;
        case TokenType::GE:        return ast::BinaryOp::Ge;
        case TokenType::AND:       return ast::BinaryOp::And;
        case TokenType::OR:        return ast::BinaryOp::Or;
        case TokenType::AMPERSAND: return ast::BinaryOp::BitAnd;
        case TokenType::PIPE:      return ast::BinaryOp::BitOr;
        case TokenType::CARET:     return ast::BinaryOp::BitXor;
        case TokenType::SHL:       return ast::BinaryOp::Shl;
        case TokenType::SHR:       return ast::BinaryOp::Shr;
        case TokenType::AT:        return ast::BinaryOp::MatMul;
        default:                   return ast::BinaryOp::Add;  // Fallback
    }
}

ast::UnaryOp Parser::token_to_unary_op(TokenType type) {
    switch (type) {
        case TokenType::MINUS: return ast::UnaryOp::Neg;
        case TokenType::NOT:   return ast::UnaryOp::Not;
        case TokenType::TILDE: return ast::UnaryOp::BitNot;
        default:               return ast::UnaryOp::Neg;  // Fallback
    }
}

} // namespace axiom
