#pragma once

/**
 * @file ast.hpp
 * @brief Abstract Syntax Tree definitions for Axiom
 * 
 * Defines all AST node types for expressions, statements, and declarations.
 */

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include "../lexer/token.hpp"

namespace axiom {
namespace ast {

// Forward declarations
struct Expr;
struct Stmt;
struct Decl;
struct Type;

// Smart pointer aliases
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using DeclPtr = std::unique_ptr<Decl>;
using TypePtr = std::unique_ptr<Type>;

// ============================================================================
// Types
// ============================================================================

/**
 * @brief Base class for all type nodes
 */
struct Type {
    SourceLocation location;
    virtual ~Type() = default;
};

/**
 * @brief Simple type like i32, str, bool
 */
struct SimpleType : Type {
    std::string name;
    
    explicit SimpleType(std::string n) : name(std::move(n)) {}
};

/**
 * @brief Generic type like List[T], Dict[K, V]
 */
struct GenericType : Type {
    std::string name;
    std::vector<TypePtr> type_args;
    
    GenericType(std::string n, std::vector<TypePtr> args)
        : name(std::move(n)), type_args(std::move(args)) {}
};

/**
 * @brief Array type like [i32; 10]
 */
struct ArrayType : Type {
    TypePtr element_type;
    std::optional<size_t> size;  // None for dynamic arrays
    
    ArrayType(TypePtr elem, std::optional<size_t> sz = std::nullopt)
        : element_type(std::move(elem)), size(sz) {}
};

/**
 * @brief Tuple type like (i32, str, f64)
 */
struct TupleType : Type {
    std::vector<TypePtr> element_types;
    
    explicit TupleType(std::vector<TypePtr> elems)
        : element_types(std::move(elems)) {}
};

/**
 * @brief Function type like fn(i32, i32) -> i32
 */
struct FunctionType : Type {
    std::vector<TypePtr> param_types;
    TypePtr return_type;
    
    FunctionType(std::vector<TypePtr> params, TypePtr ret)
        : param_types(std::move(params)), return_type(std::move(ret)) {}
};

/**
 * @brief Reference type like &T or &mut T
 */
struct ReferenceType : Type {
    TypePtr inner;
    bool is_mutable;
    
    ReferenceType(TypePtr inner_type, bool mut = false)
        : inner(std::move(inner_type)), is_mutable(mut) {}
};

// ============================================================================
// Expressions
// ============================================================================

/**
 * @brief Base class for all expression nodes
 */
struct Expr {
    SourceLocation location;
    virtual ~Expr() = default;
};

// --- Literals ---

struct IntLiteral : Expr {
    int64_t value;
    explicit IntLiteral(int64_t v) : value(v) {}
};

struct FloatLiteral : Expr {
    double value;
    explicit FloatLiteral(double v) : value(v) {}
};

struct StringLiteral : Expr {
    std::string value;
    bool is_fstring;
    explicit StringLiteral(std::string v, bool fstr = false)
        : value(std::move(v)), is_fstring(fstr) {}
};

struct BoolLiteral : Expr {
    bool value;
    explicit BoolLiteral(bool v) : value(v) {}
};

struct NoneLiteral : Expr {};

// --- Identifiers ---

struct Identifier : Expr {
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}
};

// --- Operators ---

enum class BinaryOp {
    // Arithmetic
    Add, Sub, Mul, Div, Mod, Pow,
    // Comparison
    Eq, Ne, Lt, Le, Gt, Ge,
    // Logical
    And, Or,
    // Bitwise
    BitAnd, BitOr, BitXor, Shl, Shr,
    // Matrix multiplication
    MatMul,
};

enum class UnaryOp {
    Neg,      // -x
    Not,      // not x
    BitNot,   // ~x
};

struct BinaryExpr : Expr {
    BinaryOp op;
    ExprPtr left;
    ExprPtr right;
    
    BinaryExpr(BinaryOp o, ExprPtr l, ExprPtr r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
};

struct UnaryExpr : Expr {
    UnaryOp op;
    ExprPtr operand;
    
    UnaryExpr(UnaryOp o, ExprPtr expr)
        : op(o), operand(std::move(expr)) {}
};

// --- Compound Expressions ---

struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> arguments;
    
    CallExpr(ExprPtr func, std::vector<ExprPtr> args)
        : callee(std::move(func)), arguments(std::move(args)) {}
};

struct IndexExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    
    IndexExpr(ExprPtr obj, ExprPtr idx)
        : object(std::move(obj)), index(std::move(idx)) {}
};

struct SliceExpr : Expr {
    ExprPtr object;
    std::optional<ExprPtr> start;
    std::optional<ExprPtr> end;
    std::optional<ExprPtr> step;
    
    SliceExpr(ExprPtr obj, 
              std::optional<ExprPtr> s = std::nullopt,
              std::optional<ExprPtr> e = std::nullopt,
              std::optional<ExprPtr> st = std::nullopt)
        : object(std::move(obj)), start(std::move(s)), 
          end(std::move(e)), step(std::move(st)) {}
};

struct MemberExpr : Expr {
    ExprPtr object;
    std::string member;
    
    MemberExpr(ExprPtr obj, std::string mem)
        : object(std::move(obj)), member(std::move(mem)) {}
};

struct LambdaExpr : Expr {
    struct Param {
        std::string name;
        std::optional<TypePtr> type;
    };
    std::vector<Param> params;
    std::optional<TypePtr> return_type;
    ExprPtr body;  // Single expression (block would be wrapped)
    
    LambdaExpr(std::vector<Param> p, ExprPtr b, 
               std::optional<TypePtr> ret = std::nullopt)
        : params(std::move(p)), return_type(std::move(ret)), 
          body(std::move(b)) {}
};

struct TernaryExpr : Expr {
    ExprPtr condition;
    ExprPtr then_expr;
    ExprPtr else_expr;
    
    TernaryExpr(ExprPtr cond, ExprPtr then_e, ExprPtr else_e)
        : condition(std::move(cond)), then_expr(std::move(then_e)),
          else_expr(std::move(else_e)) {}
};

// --- Collections ---

struct ListExpr : Expr {
    std::vector<ExprPtr> elements;
    explicit ListExpr(std::vector<ExprPtr> elems)
        : elements(std::move(elems)) {}
};

struct DictExpr : Expr {
    std::vector<std::pair<ExprPtr, ExprPtr>> entries;
    explicit DictExpr(std::vector<std::pair<ExprPtr, ExprPtr>> e)
        : entries(std::move(e)) {}
};

struct TupleExpr : Expr {
    std::vector<ExprPtr> elements;
    explicit TupleExpr(std::vector<ExprPtr> elems)
        : elements(std::move(elems)) {}
};

// --- Comprehensions ---

struct ListCompExpr : Expr {
    ExprPtr element;
    std::string var_name;
    ExprPtr iterable;
    std::optional<ExprPtr> condition;
    
    ListCompExpr(ExprPtr elem, std::string var, ExprPtr iter,
                 std::optional<ExprPtr> cond = std::nullopt)
        : element(std::move(elem)), var_name(std::move(var)),
          iterable(std::move(iter)), condition(std::move(cond)) {}
};

// --- Await/Spawn ---

struct AwaitExpr : Expr {
    ExprPtr operand;
    explicit AwaitExpr(ExprPtr op) : operand(std::move(op)) {}
};

// --- Assignment Expression ---

struct AssignExpr : Expr {
    ExprPtr target;
    ExprPtr value;
    std::optional<BinaryOp> compound_op;  // For += -= etc.
    
    AssignExpr(ExprPtr tgt, ExprPtr val, 
               std::optional<BinaryOp> op = std::nullopt)
        : target(std::move(tgt)), value(std::move(val)), 
          compound_op(op) {}
};

// --- Range ---

struct RangeExpr : Expr {
    ExprPtr start;
    ExprPtr end;
    bool inclusive;  // .. vs ..=
    
    RangeExpr(ExprPtr s, ExprPtr e, bool incl = false)
        : start(std::move(s)), end(std::move(e)), inclusive(incl) {}
};

// ============================================================================
// Statements
// ============================================================================

/**
 * @brief Base class for all statement nodes
 */
struct Stmt {
    SourceLocation location;
    virtual ~Stmt() = default;
};

/**
 * @brief A block of statements (used for function bodies, if/while blocks, etc.)
 */
struct Block {
    std::vector<StmtPtr> statements;
    SourceLocation location;
    
    explicit Block(std::vector<StmtPtr> stmts = {})
        : statements(std::move(stmts)) {}
};

using BlockPtr = std::unique_ptr<Block>;

// --- Expression Statement ---

struct ExprStmt : Stmt {
    ExprPtr expression;
    explicit ExprStmt(ExprPtr expr) : expression(std::move(expr)) {}
};

// --- Variable Declaration ---

struct VarDeclStmt : Stmt {
    std::string name;
    std::optional<TypePtr> type;
    std::optional<ExprPtr> initializer;
    bool is_mutable;  // let vs var
    bool is_const;    // const
    
    VarDeclStmt(std::string n, bool mut = false, bool cnst = false)
        : name(std::move(n)), is_mutable(mut), is_const(cnst) {}
};

// --- Control Flow ---

struct ReturnStmt : Stmt {
    std::optional<ExprPtr> value;
    explicit ReturnStmt(std::optional<ExprPtr> val = std::nullopt)
        : value(std::move(val)) {}
};

struct BreakStmt : Stmt {};

struct ContinueStmt : Stmt {};

struct YieldStmt : Stmt {
    ExprPtr value;
    explicit YieldStmt(ExprPtr val) : value(std::move(val)) {}
};

// --- If Statement ---

struct IfStmt : Stmt {
    ExprPtr condition;
    BlockPtr then_block;
    std::vector<std::pair<ExprPtr, BlockPtr>> elif_blocks;  // elif chains
    std::optional<BlockPtr> else_block;
    
    IfStmt(ExprPtr cond, BlockPtr then_b)
        : condition(std::move(cond)), then_block(std::move(then_b)) {}
};

// --- While Loop ---

struct WhileStmt : Stmt {
    ExprPtr condition;
    BlockPtr body;
    
    WhileStmt(ExprPtr cond, BlockPtr b)
        : condition(std::move(cond)), body(std::move(b)) {}
};

// --- For Loop ---

struct ForStmt : Stmt {
    std::string variable;
    ExprPtr iterable;
    BlockPtr body;
    
    ForStmt(std::string var, ExprPtr iter, BlockPtr b)
        : variable(std::move(var)), iterable(std::move(iter)), 
          body(std::move(b)) {}
};

// --- Match Statement ---

struct MatchArm {
    ExprPtr pattern;  // Simplified: just expression for now
    std::optional<ExprPtr> guard;  // Optional guard condition
    BlockPtr body;
};

struct MatchStmt : Stmt {
    ExprPtr value;
    std::vector<MatchArm> arms;
    
    MatchStmt(ExprPtr val, std::vector<MatchArm> a)
        : value(std::move(val)), arms(std::move(a)) {}
};

// ============================================================================
// Declarations
// ============================================================================

/**
 * @brief Base class for all declaration nodes
 */
struct Decl {
    SourceLocation location;
    bool is_public = false;
    virtual ~Decl() = default;
};

// --- Function Parameter ---

struct FnParam {
    std::string name;
    TypePtr type;
    std::optional<ExprPtr> default_value;
    bool is_mutable = false;
    
    FnParam(std::string n, TypePtr t)
        : name(std::move(n)), type(std::move(t)) {}
};

// --- Function Declaration ---

struct FnDecl : Decl {
    std::string name;
    std::vector<FnParam> params;
    std::optional<TypePtr> return_type;
    BlockPtr body;
    bool is_async = false;
    
    // Generic type parameters
    std::vector<std::string> type_params;
    
    FnDecl(std::string n) : name(std::move(n)) {}
};

// --- Struct Field ---

struct StructField {
    std::string name;
    TypePtr type;
    std::optional<ExprPtr> default_value;
    bool is_public = false;
};

// --- Struct Declaration ---

struct StructDecl : Decl {
    std::string name;
    std::vector<std::string> type_params;  // Generics
    std::vector<StructField> fields;
    std::vector<std::unique_ptr<FnDecl>> methods;
    
    explicit StructDecl(std::string n) : name(std::move(n)) {}
};

// --- Class Declaration (with inheritance) ---

struct ClassDecl : Decl {
    std::string name;
    std::optional<std::string> base_class;
    std::vector<std::string> type_params;
    std::vector<StructField> fields;
    std::vector<std::unique_ptr<FnDecl>> methods;
    
    explicit ClassDecl(std::string n) : name(std::move(n)) {}
};

// --- Trait Declaration ---

struct TraitDecl : Decl {
    std::string name;
    std::vector<std::string> type_params;
    std::vector<std::unique_ptr<FnDecl>> methods;  // May have default implementations
    
    explicit TraitDecl(std::string n) : name(std::move(n)) {}
};

// --- Impl Block ---

struct ImplDecl : Decl {
    std::optional<std::string> trait_name;  // None for inherent impl
    std::string type_name;
    std::vector<std::unique_ptr<FnDecl>> methods;
    
    explicit ImplDecl(std::string type) : type_name(std::move(type)) {}
};

// --- Enum Declaration ---

struct EnumVariant {
    std::string name;
    std::vector<TypePtr> fields;  // Tuple variant
};

struct EnumDecl : Decl {
    std::string name;
    std::vector<std::string> type_params;
    std::vector<EnumVariant> variants;
    
    explicit EnumDecl(std::string n) : name(std::move(n)) {}
};

// --- Type Alias ---

struct TypeAliasDecl : Decl {
    std::string name;
    TypePtr aliased_type;
    
    TypeAliasDecl(std::string n, TypePtr t)
        : name(std::move(n)), aliased_type(std::move(t)) {}
};

// --- Import ---

struct ImportDecl : Decl {
    std::string module_path;  // e.g., "std.collections"
    std::optional<std::string> alias;  // import x as y
    std::vector<std::string> symbols;  // from x import a, b, c
    bool import_all = false;  // from x import *
    
    explicit ImportDecl(std::string path) : module_path(std::move(path)) {}
};

// ============================================================================
// Program (Root)
// ============================================================================

/**
 * @brief Root AST node representing an entire Axiom source file
 */
struct Program {
    std::vector<DeclPtr> declarations;
    std::string filename;
    
    explicit Program(std::string file = "") : filename(std::move(file)) {}
};

} // namespace ast
} // namespace axiom
