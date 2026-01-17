#pragma once

/**
 * @file core.hpp
 * @brief Core standard library types for Axiom
 * 
 * Provides List, Dict, String, Option, Result types.
 */

#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <variant>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cstdint>

namespace axiom {
namespace stdlib {

// Forward declarations
template<typename T> class List;
template<typename K, typename V> class Dict;
template<typename T> class Option;
template<typename T, typename E> class Result;

/**
 * @brief Axiom List - Dynamic array with Python-like interface
 */
template<typename T>
class List {
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    
    List() = default;
    List(std::initializer_list<T> init) : data_(init) {}
    List(const std::vector<T>& vec) : data_(vec) {}
    List(std::vector<T>&& vec) : data_(std::move(vec)) {}
    
    // Element access
    T& operator[](int64_t index) {
        return data_[normalize_index(index)];
    }
    
    const T& operator[](int64_t index) const {
        return data_[normalize_index(index)];
    }
    
    T& at(int64_t index) {
        size_t idx = normalize_index(index);
        if (idx >= data_.size()) {
            throw std::out_of_range("List index out of range");
        }
        return data_[idx];
    }
    
    // Modifiers
    void append(const T& value) { data_.push_back(value); }
    void append(T&& value) { data_.push_back(std::move(value)); }
    
    void insert(int64_t index, const T& value) {
        data_.insert(data_.begin() + normalize_index(index), value);
    }
    
    T pop(int64_t index = -1) {
        size_t idx = normalize_index(index);
        T value = std::move(data_[idx]);
        data_.erase(data_.begin() + idx);
        return value;
    }
    
    void remove(const T& value) {
        auto it = std::find(data_.begin(), data_.end(), value);
        if (it != data_.end()) {
            data_.erase(it);
        }
    }
    
    void clear() { data_.clear(); }
    
    void extend(const List<T>& other) {
        data_.insert(data_.end(), other.begin(), other.end());
    }
    
    void reverse() {
        std::reverse(data_.begin(), data_.end());
    }
    
    void sort() {
        std::sort(data_.begin(), data_.end());
    }
    
    // Queries
    int64_t len() const { return static_cast<int64_t>(data_.size()); }
    bool empty() const { return data_.empty(); }
    
    bool contains(const T& value) const {
        return std::find(data_.begin(), data_.end(), value) != data_.end();
    }
    
    int64_t index(const T& value) const {
        auto it = std::find(data_.begin(), data_.end(), value);
        if (it == data_.end()) return -1;
        return static_cast<int64_t>(it - data_.begin());
    }
    
    int64_t count(const T& value) const {
        return std::count(data_.begin(), data_.end(), value);
    }
    
    // Slicing
    List<T> slice(int64_t start, int64_t end) const {
        size_t s = normalize_index(start);
        size_t e = normalize_index(end);
        if (s > e) std::swap(s, e);
        return List<T>(std::vector<T>(data_.begin() + s, data_.begin() + e));
    }
    
    // Functional operations
    template<typename F>
    auto map(F&& func) const -> List<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));
        List<U> result;
        for (const auto& item : data_) {
            result.append(func(item));
        }
        return result;
    }
    
    template<typename F>
    List<T> filter(F&& pred) const {
        List<T> result;
        for (const auto& item : data_) {
            if (pred(item)) {
                result.append(item);
            }
        }
        return result;
    }
    
    template<typename U, typename F>
    U reduce(U init, F&& func) const {
        U acc = init;
        for (const auto& item : data_) {
            acc = func(acc, item);
        }
        return acc;
    }
    
    // Iterators
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    
    // Conversion
    std::vector<T>& to_vector() { return data_; }
    const std::vector<T>& to_vector() const { return data_; }
    
    std::string to_string() const {
        std::ostringstream ss;
        ss << "[";
        for (size_t i = 0; i < data_.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << data_[i];
        }
        ss << "]";
        return ss.str();
    }

private:
    std::vector<T> data_;
    
    size_t normalize_index(int64_t index) const {
        if (index < 0) {
            index += static_cast<int64_t>(data_.size());
        }
        return static_cast<size_t>(index);
    }
};

/**
 * @brief Axiom Dict - Hash map with Python-like interface
 */
template<typename K, typename V>
class Dict {
public:
    using iterator = typename std::unordered_map<K, V>::iterator;
    using const_iterator = typename std::unordered_map<K, V>::const_iterator;
    
    Dict() = default;
    Dict(std::initializer_list<std::pair<const K, V>> init) : data_(init) {}
    
    // Element access
    V& operator[](const K& key) { return data_[key]; }
    
    V& at(const K& key) { return data_.at(key); }
    const V& at(const K& key) const { return data_.at(key); }
    
    V get(const K& key, const V& default_value = V{}) const {
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : default_value;
    }
    
    // Modifiers
    void set(const K& key, const V& value) { data_[key] = value; }
    void set(const K& key, V&& value) { data_[key] = std::move(value); }
    
    bool remove(const K& key) {
        return data_.erase(key) > 0;
    }
    
    void clear() { data_.clear(); }
    
    void update(const Dict<K, V>& other) {
        for (const auto& [k, v] : other) {
            data_[k] = v;
        }
    }
    
    // Queries
    int64_t len() const { return static_cast<int64_t>(data_.size()); }
    bool empty() const { return data_.empty(); }
    
    bool contains(const K& key) const {
        return data_.count(key) > 0;
    }
    
    List<K> keys() const {
        List<K> result;
        for (const auto& [k, v] : data_) {
            result.append(k);
        }
        return result;
    }
    
    List<V> values() const {
        List<V> result;
        for (const auto& [k, v] : data_) {
            result.append(v);
        }
        return result;
    }
    
    List<std::pair<K, V>> items() const {
        List<std::pair<K, V>> result;
        for (const auto& [k, v] : data_) {
            result.append({k, v});
        }
        return result;
    }
    
    // Iterators
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    
    // Conversion
    std::unordered_map<K, V>& to_map() { return data_; }
    const std::unordered_map<K, V>& to_map() const { return data_; }

private:
    std::unordered_map<K, V> data_;
};

/**
 * @brief Axiom Option - Optional value (like Rust's Option)
 */
template<typename T>
class Option {
public:
    Option() : value_(std::nullopt) {}
    Option(std::nullopt_t) : value_(std::nullopt) {}
    Option(const T& value) : value_(value) {}
    Option(T&& value) : value_(std::move(value)) {}
    
    static Option<T> some(const T& value) { return Option<T>(value); }
    static Option<T> none() { return Option<T>(); }
    
    bool is_some() const { return value_.has_value(); }
    bool is_none() const { return !value_.has_value(); }
    
    T& unwrap() {
        if (!value_) throw std::runtime_error("Called unwrap on None");
        return *value_;
    }
    
    const T& unwrap() const {
        if (!value_) throw std::runtime_error("Called unwrap on None");
        return *value_;
    }
    
    T unwrap_or(const T& default_value) const {
        return value_.value_or(default_value);
    }
    
    template<typename F>
    T unwrap_or_else(F&& func) const {
        return value_.has_value() ? *value_ : func();
    }
    
    template<typename F>
    auto map(F&& func) const -> Option<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));
        if (value_) {
            return Option<U>(func(*value_));
        }
        return Option<U>();
    }
    
    template<typename F>
    auto and_then(F&& func) const -> decltype(func(std::declval<T>())) {
        if (value_) {
            return func(*value_);
        }
        return decltype(func(std::declval<T>()))();
    }
    
    explicit operator bool() const { return is_some(); }

private:
    std::optional<T> value_;
};

/**
 * @brief Unit type for void results
 */
struct Unit {};

/**
 * @brief Axiom Result - Error handling (like Rust's Result)
 * Uses a tagged approach to avoid variant issues when T == E
 */
template<typename T, typename E = std::string>
class Result {
public:
    Result() : is_ok_(false), value_(), error_() {}
    
    static Result<T, E> ok(const T& value) {
        Result<T, E> r;
        r.is_ok_ = true;
        r.value_ = value;
        return r;
    }
    
    static Result<T, E> ok(T&& value) {
        Result<T, E> r;
        r.is_ok_ = true;
        r.value_ = std::move(value);
        return r;
    }
    
    static Result<T, E> err(const E& error) {
        Result<T, E> r;
        r.is_ok_ = false;
        r.error_ = error;
        return r;
    }
    
    static Result<T, E> err(E&& error) {
        Result<T, E> r;
        r.is_ok_ = false;
        r.error_ = std::move(error);
        return r;
    }
    
    bool is_ok() const { return is_ok_; }
    bool is_err() const { return !is_ok_; }
    
    T& unwrap() {
        if (!is_ok_) {
            throw std::runtime_error("Called unwrap on Err");
        }
        return value_;
    }
    
    const T& unwrap() const {
        if (!is_ok_) {
            throw std::runtime_error("Called unwrap on Err");
        }
        return value_;
    }
    
    E& unwrap_err() {
        if (is_ok_) {
            throw std::runtime_error("Called unwrap_err on Ok");
        }
        return error_;
    }
    
    const E& unwrap_err() const {
        if (is_ok_) {
            throw std::runtime_error("Called unwrap_err on Ok");
        }
        return error_;
    }
    
    T unwrap_or(const T& default_value) const {
        return is_ok_ ? value_ : default_value;
    }
    
    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<T>())), E> {
        using U = decltype(func(std::declval<T>()));
        if (is_ok_) {
            return Result<U, E>::ok(func(value_));
        }
        return Result<U, E>::err(error_);
    }
    
    explicit operator bool() const { return is_ok_; }

private:
    bool is_ok_;
    T value_;
    E error_;
};

/**
 * @brief Extended string with Python-like methods
 */
class String {
public:
    String() = default;
    String(const char* s) : data_(s) {}
    String(const std::string& s) : data_(s) {}
    String(std::string&& s) : data_(std::move(s)) {}
    
    // Basic operations
    int64_t len() const { return static_cast<int64_t>(data_.size()); }
    bool empty() const { return data_.empty(); }
    
    char& operator[](int64_t index) {
        return data_[normalize_index(index)];
    }
    
    // String operations
    String upper() const {
        String result = *this;
        std::transform(result.data_.begin(), result.data_.end(), 
                      result.data_.begin(), ::toupper);
        return result;
    }
    
    String lower() const {
        String result = *this;
        std::transform(result.data_.begin(), result.data_.end(), 
                      result.data_.begin(), ::tolower);
        return result;
    }
    
    String strip() const {
        size_t start = data_.find_first_not_of(" \t\n\r");
        size_t end = data_.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) return String("");
        return String(data_.substr(start, end - start + 1));
    }
    
    bool startswith(const String& prefix) const {
        return data_.rfind(prefix.data_, 0) == 0;
    }
    
    bool endswith(const String& suffix) const {
        if (suffix.len() > len()) return false;
        return data_.compare(len() - suffix.len(), suffix.len(), suffix.data_) == 0;
    }
    
    bool contains(const String& sub) const {
        return data_.find(sub.data_) != std::string::npos;
    }
    
    int64_t find(const String& sub, int64_t start = 0) const {
        size_t pos = data_.find(sub.data_, start);
        return (pos == std::string::npos) ? -1 : static_cast<int64_t>(pos);
    }
    
    String replace(const String& old, const String& new_) const {
        std::string result = data_;
        size_t pos = 0;
        while ((pos = result.find(old.data_, pos)) != std::string::npos) {
            result.replace(pos, old.len(), new_.data_);
            pos += new_.len();
        }
        return String(result);
    }
    
    List<String> split(const String& sep = " ") const {
        List<String> result;
        size_t start = 0, end;
        while ((end = data_.find(sep.data_, start)) != std::string::npos) {
            result.append(String(data_.substr(start, end - start)));
            start = end + sep.len();
        }
        result.append(String(data_.substr(start)));
        return result;
    }
    
    static String join(const String& sep, const List<String>& parts) {
        if (parts.empty()) return String("");
        std::ostringstream ss;
        for (int64_t i = 0; i < parts.len(); ++i) {
            if (i > 0) ss << sep.data_;
            ss << parts[i].data_;
        }
        return String(ss.str());
    }
    
    // Conversion
    const std::string& to_std() const { return data_; }
    const char* c_str() const { return data_.c_str(); }
    
    // Operators
    String operator+(const String& other) const {
        return String(data_ + other.data_);
    }
    
    bool operator==(const String& other) const { return data_ == other.data_; }
    bool operator!=(const String& other) const { return data_ != other.data_; }
    bool operator<(const String& other) const { return data_ < other.data_; }

private:
    std::string data_;
    
    size_t normalize_index(int64_t index) const {
        if (index < 0) {
            index += static_cast<int64_t>(data_.size());
        }
        return static_cast<size_t>(index);
    }
};

// Convenience functions
template<typename T>
inline int64_t len(const List<T>& list) { return list.len(); }

template<typename K, typename V>
inline int64_t len(const Dict<K, V>& dict) { return dict.len(); }

inline int64_t len(const String& s) { return s.len(); }

} // namespace stdlib
} // namespace axiom
