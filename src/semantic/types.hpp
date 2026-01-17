#pragma once

/**
 * @file types.hpp
 * @brief Type system for Axiom semantic analysis
 * 
 * Defines semantic types used during type checking, separate from AST type nodes.
 */

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>

namespace axiom {
namespace semantic {

// Forward declarations
struct SemanticType;
using TypePtr = std::shared_ptr<SemanticType>;

/**
 * @brief Type kind enumeration
 */
enum class TypeKind {
    // Primitives
    Void,
    Bool,
    Int8, Int16, Int32, Int64,
    UInt8, UInt16, UInt32, UInt64,
    Float32, Float64,
    Char,
    String,
    
    // Composite
    Array,
    List,
    Dict,
    Tuple,
    Function,
    Struct,
    Class,
    Enum,
    Trait,
    
    // Special
    Reference,
    Optional,  // T?
    Result,    // Result[T, E]
    Generic,   // Unresolved generic parameter
    TypeVar,   // Type variable for inference
    Never,     // ! type (never returns)
    Unknown,   // Unresolved/error type
};

/**
 * @brief Base semantic type
 */
struct SemanticType {
    TypeKind kind;
    std::string name;
    bool is_mutable = false;
    
    SemanticType(TypeKind k, std::string n = "") : kind(k), name(std::move(n)) {}
    virtual ~SemanticType() = default;
    
    virtual bool equals(const SemanticType& other) const;
    virtual std::string to_string() const;
    virtual TypePtr clone() const;
    
    bool is_numeric() const;
    bool is_integer() const;
    bool is_float() const;
    bool is_primitive() const;
};

// === Primitive Types (Singletons) ===

TypePtr void_type();
TypePtr bool_type();
TypePtr i8_type();
TypePtr i16_type();
TypePtr i32_type();
TypePtr i64_type();
TypePtr u8_type();
TypePtr u16_type();
TypePtr u32_type();
TypePtr u64_type();
TypePtr f32_type();
TypePtr f64_type();
TypePtr char_type();
TypePtr string_type();
TypePtr never_type();
TypePtr unknown_type();

// === Composite Types ===

struct ArrayType : SemanticType {
    TypePtr element_type;
    std::optional<size_t> size;  // None for dynamic
    
    ArrayType(TypePtr elem, std::optional<size_t> sz = std::nullopt)
        : SemanticType(TypeKind::Array), element_type(std::move(elem)), size(sz) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct ListType : SemanticType {
    TypePtr element_type;
    
    explicit ListType(TypePtr elem)
        : SemanticType(TypeKind::List), element_type(std::move(elem)) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct DictType : SemanticType {
    TypePtr key_type;
    TypePtr value_type;
    
    DictType(TypePtr key, TypePtr val)
        : SemanticType(TypeKind::Dict), key_type(std::move(key)), value_type(std::move(val)) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct TupleType : SemanticType {
    std::vector<TypePtr> element_types;
    
    explicit TupleType(std::vector<TypePtr> elems)
        : SemanticType(TypeKind::Tuple), element_types(std::move(elems)) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct FunctionType : SemanticType {
    std::vector<TypePtr> param_types;
    TypePtr return_type;
    bool is_async = false;
    
    FunctionType(std::vector<TypePtr> params, TypePtr ret)
        : SemanticType(TypeKind::Function), 
          param_types(std::move(params)), return_type(std::move(ret)) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct ReferenceType : SemanticType {
    TypePtr inner_type;
    bool is_mut;
    
    ReferenceType(TypePtr inner, bool mut = false)
        : SemanticType(TypeKind::Reference), inner_type(std::move(inner)), is_mut(mut) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct OptionalType : SemanticType {
    TypePtr inner_type;
    
    explicit OptionalType(TypePtr inner)
        : SemanticType(TypeKind::Optional), inner_type(std::move(inner)) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct ResultType : SemanticType {
    TypePtr ok_type;
    TypePtr err_type;
    
    ResultType(TypePtr ok, TypePtr err)
        : SemanticType(TypeKind::Result), ok_type(std::move(ok)), err_type(std::move(err)) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

// === User-Defined Types ===

struct StructField {
    std::string name;
    TypePtr type;
    bool is_public = false;
};

struct StructType : SemanticType {
    std::vector<StructField> fields;
    std::vector<std::string> type_params;
    std::unordered_map<std::string, TypePtr> type_args;  // Resolved generics
    
    explicit StructType(std::string name)
        : SemanticType(TypeKind::Struct, std::move(name)) {}
    
    TypePtr get_field_type(const std::string& field_name) const;
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct ClassType : SemanticType {
    std::vector<StructField> fields;
    std::optional<std::string> base_class;
    std::vector<std::string> type_params;
    
    explicit ClassType(std::string name)
        : SemanticType(TypeKind::Class, std::move(name)) {}
    
    TypePtr get_field_type(const std::string& field_name) const;
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct EnumVariant {
    std::string name;
    std::vector<TypePtr> fields;  // Tuple variant fields
};

struct EnumType : SemanticType {
    std::vector<EnumVariant> variants;
    std::vector<std::string> type_params;
    
    explicit EnumType(std::string name)
        : SemanticType(TypeKind::Enum, std::move(name)) {}
    
    bool has_variant(const std::string& name) const;
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct TraitType : SemanticType {
    std::vector<FunctionType> methods;
    std::vector<std::string> type_params;
    
    explicit TraitType(std::string name)
        : SemanticType(TypeKind::Trait, std::move(name)) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

// === Generic / Type Variable ===

struct GenericType : SemanticType {
    std::vector<TypePtr> constraints;  // Trait bounds
    
    explicit GenericType(std::string name)
        : SemanticType(TypeKind::Generic, std::move(name)) {}
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

struct TypeVariable : SemanticType {
    size_t id;
    TypePtr resolved;  // Set when unified
    
    explicit TypeVariable(size_t id_)
        : SemanticType(TypeKind::TypeVar), id(id_) {}
    
    bool is_resolved() const { return resolved != nullptr; }
    TypePtr resolve() const { return resolved ? resolved : nullptr; }
    
    bool equals(const SemanticType& other) const override;
    std::string to_string() const override;
    TypePtr clone() const override;
};

// === Type Utilities ===

/**
 * @brief Check if two types are compatible (one can be assigned to the other)
 */
bool is_assignable(const TypePtr& from, const TypePtr& to);

/**
 * @brief Find common type of two types (for binary operations)
 */
TypePtr common_type(const TypePtr& a, const TypePtr& b);

/**
 * @brief Substitute type parameters with concrete types
 */
TypePtr substitute(const TypePtr& type, 
                   const std::unordered_map<std::string, TypePtr>& substitutions);

} // namespace semantic
} // namespace axiom
