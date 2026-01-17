/**
 * @file types.cpp
 * @brief Type system implementation for Axiom
 */

#include "types.hpp"
#include <sstream>

namespace axiom {
namespace semantic {

// === Singleton Primitive Types ===

namespace {
    TypePtr make_primitive(TypeKind kind, const std::string& name) {
        return std::make_shared<SemanticType>(kind, name);
    }
}

TypePtr void_type() {
    static auto type = make_primitive(TypeKind::Void, "void");
    return type;
}

TypePtr bool_type() {
    static auto type = make_primitive(TypeKind::Bool, "bool");
    return type;
}

TypePtr i8_type() {
    static auto type = make_primitive(TypeKind::Int8, "i8");
    return type;
}

TypePtr i16_type() {
    static auto type = make_primitive(TypeKind::Int16, "i16");
    return type;
}

TypePtr i32_type() {
    static auto type = make_primitive(TypeKind::Int32, "i32");
    return type;
}

TypePtr i64_type() {
    static auto type = make_primitive(TypeKind::Int64, "i64");
    return type;
}

TypePtr u8_type() {
    static auto type = make_primitive(TypeKind::UInt8, "u8");
    return type;
}

TypePtr u16_type() {
    static auto type = make_primitive(TypeKind::UInt16, "u16");
    return type;
}

TypePtr u32_type() {
    static auto type = make_primitive(TypeKind::UInt32, "u32");
    return type;
}

TypePtr u64_type() {
    static auto type = make_primitive(TypeKind::UInt64, "u64");
    return type;
}

TypePtr f32_type() {
    static auto type = make_primitive(TypeKind::Float32, "f32");
    return type;
}

TypePtr f64_type() {
    static auto type = make_primitive(TypeKind::Float64, "f64");
    return type;
}

TypePtr char_type() {
    static auto type = make_primitive(TypeKind::Char, "char");
    return type;
}

TypePtr string_type() {
    static auto type = make_primitive(TypeKind::String, "str");
    return type;
}

TypePtr never_type() {
    static auto type = make_primitive(TypeKind::Never, "!");
    return type;
}

TypePtr unknown_type() {
    static auto type = make_primitive(TypeKind::Unknown, "?");
    return type;
}

// === SemanticType Base ===

bool SemanticType::equals(const SemanticType& other) const {
    return kind == other.kind && name == other.name;
}

std::string SemanticType::to_string() const {
    return name.empty() ? "unknown" : name;
}

TypePtr SemanticType::clone() const {
    return std::make_shared<SemanticType>(kind, name);
}

bool SemanticType::is_numeric() const {
    return is_integer() || is_float();
}

bool SemanticType::is_integer() const {
    return kind >= TypeKind::Int8 && kind <= TypeKind::UInt64;
}

bool SemanticType::is_float() const {
    return kind == TypeKind::Float32 || kind == TypeKind::Float64;
}

bool SemanticType::is_primitive() const {
    return kind >= TypeKind::Void && kind <= TypeKind::String;
}

// === ArrayType ===

bool ArrayType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Array) return false;
    auto& o = static_cast<const ArrayType&>(other);
    return element_type->equals(*o.element_type) && size == o.size;
}

std::string ArrayType::to_string() const {
    std::ostringstream ss;
    ss << "[" << element_type->to_string();
    if (size) ss << "; " << *size;
    ss << "]";
    return ss.str();
}

TypePtr ArrayType::clone() const {
    return std::make_shared<ArrayType>(element_type->clone(), size);
}

// === ListType ===

bool ListType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::List) return false;
    auto& o = static_cast<const ListType&>(other);
    return element_type->equals(*o.element_type);
}

std::string ListType::to_string() const {
    return "List[" + element_type->to_string() + "]";
}

TypePtr ListType::clone() const {
    return std::make_shared<ListType>(element_type->clone());
}

// === DictType ===

bool DictType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Dict) return false;
    auto& o = static_cast<const DictType&>(other);
    return key_type->equals(*o.key_type) && value_type->equals(*o.value_type);
}

std::string DictType::to_string() const {
    return "Dict[" + key_type->to_string() + ", " + value_type->to_string() + "]";
}

TypePtr DictType::clone() const {
    return std::make_shared<DictType>(key_type->clone(), value_type->clone());
}

// === TupleType ===

bool TupleType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Tuple) return false;
    auto& o = static_cast<const TupleType&>(other);
    if (element_types.size() != o.element_types.size()) return false;
    for (size_t i = 0; i < element_types.size(); ++i) {
        if (!element_types[i]->equals(*o.element_types[i])) return false;
    }
    return true;
}

std::string TupleType::to_string() const {
    std::ostringstream ss;
    ss << "(";
    for (size_t i = 0; i < element_types.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << element_types[i]->to_string();
    }
    ss << ")";
    return ss.str();
}

TypePtr TupleType::clone() const {
    std::vector<TypePtr> cloned;
    for (const auto& t : element_types) {
        cloned.push_back(t->clone());
    }
    return std::make_shared<TupleType>(std::move(cloned));
}

// === FunctionType ===

bool FunctionType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Function) return false;
    auto& o = static_cast<const FunctionType&>(other);
    if (param_types.size() != o.param_types.size()) return false;
    if (!return_type->equals(*o.return_type)) return false;
    for (size_t i = 0; i < param_types.size(); ++i) {
        if (!param_types[i]->equals(*o.param_types[i])) return false;
    }
    return is_async == o.is_async;
}

std::string FunctionType::to_string() const {
    std::ostringstream ss;
    if (is_async) ss << "async ";
    ss << "fn(";
    for (size_t i = 0; i < param_types.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << param_types[i]->to_string();
    }
    ss << ") -> " << return_type->to_string();
    return ss.str();
}

TypePtr FunctionType::clone() const {
    std::vector<TypePtr> cloned_params;
    for (const auto& t : param_types) {
        cloned_params.push_back(t->clone());
    }
    auto fn = std::make_shared<FunctionType>(std::move(cloned_params), return_type->clone());
    fn->is_async = is_async;
    return fn;
}

// === ReferenceType ===

bool ReferenceType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Reference) return false;
    auto& o = static_cast<const ReferenceType&>(other);
    return inner_type->equals(*o.inner_type) && is_mut == o.is_mut;
}

std::string ReferenceType::to_string() const {
    return (is_mut ? "&mut " : "&") + inner_type->to_string();
}

TypePtr ReferenceType::clone() const {
    return std::make_shared<ReferenceType>(inner_type->clone(), is_mut);
}

// === OptionalType ===

bool OptionalType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Optional) return false;
    auto& o = static_cast<const OptionalType&>(other);
    return inner_type->equals(*o.inner_type);
}

std::string OptionalType::to_string() const {
    return inner_type->to_string() + "?";
}

TypePtr OptionalType::clone() const {
    return std::make_shared<OptionalType>(inner_type->clone());
}

// === ResultType ===

bool ResultType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Result) return false;
    auto& o = static_cast<const ResultType&>(other);
    return ok_type->equals(*o.ok_type) && err_type->equals(*o.err_type);
}

std::string ResultType::to_string() const {
    return "Result[" + ok_type->to_string() + ", " + err_type->to_string() + "]";
}

TypePtr ResultType::clone() const {
    return std::make_shared<ResultType>(ok_type->clone(), err_type->clone());
}

// === StructType ===

TypePtr StructType::get_field_type(const std::string& field_name) const {
    for (const auto& field : fields) {
        if (field.name == field_name) return field.type;
    }
    return nullptr;
}

bool StructType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Struct) return false;
    return name == other.name;  // Struct equality is by name
}

std::string StructType::to_string() const {
    return name;
}

TypePtr StructType::clone() const {
    auto s = std::make_shared<StructType>(name);
    s->fields = fields;
    s->type_params = type_params;
    s->type_args = type_args;
    return s;
}

// === ClassType ===

TypePtr ClassType::get_field_type(const std::string& field_name) const {
    for (const auto& field : fields) {
        if (field.name == field_name) return field.type;
    }
    return nullptr;
}

bool ClassType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Class) return false;
    return name == other.name;
}

std::string ClassType::to_string() const {
    return name;
}

TypePtr ClassType::clone() const {
    auto c = std::make_shared<ClassType>(name);
    c->fields = fields;
    c->base_class = base_class;
    c->type_params = type_params;
    return c;
}

// === EnumType ===

bool EnumType::has_variant(const std::string& variant_name) const {
    for (const auto& v : variants) {
        if (v.name == variant_name) return true;
    }
    return false;
}

bool EnumType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Enum) return false;
    return name == other.name;
}

std::string EnumType::to_string() const {
    return name;
}

TypePtr EnumType::clone() const {
    auto e = std::make_shared<EnumType>(name);
    e->variants = variants;
    e->type_params = type_params;
    return e;
}

// === TraitType ===

bool TraitType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Trait) return false;
    return name == other.name;
}

std::string TraitType::to_string() const {
    return name;
}

TypePtr TraitType::clone() const {
    auto t = std::make_shared<TraitType>(name);
    t->methods = methods;
    t->type_params = type_params;
    return t;
}

// === GenericType ===

bool GenericType::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::Generic) return false;
    return name == other.name;
}

std::string GenericType::to_string() const {
    return name;
}

TypePtr GenericType::clone() const {
    auto g = std::make_shared<GenericType>(name);
    g->constraints = constraints;
    return g;
}

// === TypeVariable ===

bool TypeVariable::equals(const SemanticType& other) const {
    if (other.kind != TypeKind::TypeVar) return false;
    auto& o = static_cast<const TypeVariable&>(other);
    if (resolved && o.resolved) {
        return resolved->equals(*o.resolved);
    }
    return id == o.id;
}

std::string TypeVariable::to_string() const {
    if (resolved) return resolved->to_string();
    return "T" + std::to_string(id);
}

TypePtr TypeVariable::clone() const {
    auto tv = std::make_shared<TypeVariable>(id);
    if (resolved) tv->resolved = resolved->clone();
    return tv;
}

// === Type Utilities ===

bool is_assignable(const TypePtr& from, const TypePtr& to) {
    if (!from || !to) return false;
    
    // Same type
    if (from->equals(*to)) return true;
    
    // Never type is assignable to anything
    if (from->kind == TypeKind::Never) return true;
    
    // Numeric widening
    if (from->is_integer() && to->is_integer()) {
        // Allow smaller int to larger int (same signedness)
        return true;  // Simplified - full impl would check sizes
    }
    
    if (from->is_integer() && to->is_float()) {
        return true;  // Int to float is always ok
    }
    
    // Optional types
    if (to->kind == TypeKind::Optional) {
        auto& opt = static_cast<const OptionalType&>(*to);
        return is_assignable(from, opt.inner_type);
    }
    
    // Reference compatibility
    if (to->kind == TypeKind::Reference) {
        auto& ref = static_cast<const ReferenceType&>(*to);
        if (ref.is_mut) {
            // Mutable ref requires exact match
            return from->equals(*ref.inner_type);
        }
        return is_assignable(from, ref.inner_type);
    }
    
    return false;
}

TypePtr common_type(const TypePtr& a, const TypePtr& b) {
    if (!a || !b) return unknown_type();
    
    if (a->equals(*b)) return a;
    
    // Numeric promotion
    if (a->is_float() || b->is_float()) {
        if (a->kind == TypeKind::Float64 || b->kind == TypeKind::Float64) {
            return f64_type();
        }
        return f32_type();
    }
    
    if (a->is_integer() && b->is_integer()) {
        // Return wider integer type
        return i64_type();  // Simplified
    }
    
    return unknown_type();
}

TypePtr substitute(const TypePtr& type,
                   const std::unordered_map<std::string, TypePtr>& substitutions) {
    if (!type) return nullptr;
    
    // Generic type - substitute if in map
    if (type->kind == TypeKind::Generic) {
        auto it = substitutions.find(type->name);
        if (it != substitutions.end()) {
            return it->second;
        }
        return type;
    }
    
    // Recursively substitute in composite types
    if (type->kind == TypeKind::Array) {
        auto& arr = static_cast<const ArrayType&>(*type);
        return std::make_shared<ArrayType>(
            substitute(arr.element_type, substitutions), arr.size
        );
    }
    
    if (type->kind == TypeKind::List) {
        auto& list = static_cast<const ListType&>(*type);
        return std::make_shared<ListType>(
            substitute(list.element_type, substitutions)
        );
    }
    
    if (type->kind == TypeKind::Dict) {
        auto& dict = static_cast<const DictType&>(*type);
        return std::make_shared<DictType>(
            substitute(dict.key_type, substitutions),
            substitute(dict.value_type, substitutions)
        );
    }
    
    if (type->kind == TypeKind::Tuple) {
        auto& tuple = static_cast<const TupleType&>(*type);
        std::vector<TypePtr> elems;
        for (const auto& e : tuple.element_types) {
            elems.push_back(substitute(e, substitutions));
        }
        return std::make_shared<TupleType>(std::move(elems));
    }
    
    if (type->kind == TypeKind::Function) {
        auto& fn = static_cast<const FunctionType&>(*type);
        std::vector<TypePtr> params;
        for (const auto& p : fn.param_types) {
            params.push_back(substitute(p, substitutions));
        }
        return std::make_shared<FunctionType>(
            std::move(params),
            substitute(fn.return_type, substitutions)
        );
    }
    
    return type;
}

} // namespace semantic
} // namespace axiom
