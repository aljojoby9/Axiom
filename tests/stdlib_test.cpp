/**
 * @file stdlib_test.cpp
 * @brief Unit tests for Axiom standard library
 */

#include <iostream>
#include <cassert>
#include "stdlib/stdlib.hpp"

using namespace axiom::stdlib;
using namespace axiom::stdlib::math;
using namespace axiom::stdlib::io;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "... "; \
    test_##name(); \
    std::cout << "PASSED\n"; \
} while(0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        std::cerr << "\nAssertion failed: " << #x << "\n"; \
        std::exit(1); \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))

// === List Tests ===

TEST(list_basic) {
    List<int> nums = {1, 2, 3, 4, 5};
    ASSERT_EQ(nums.len(), 5);
    ASSERT_EQ(nums[0], 1);
    ASSERT_EQ(nums[-1], 5);  // Negative indexing
}

TEST(list_append_pop) {
    List<int> nums;
    nums.append(10);
    nums.append(20);
    ASSERT_EQ(nums.len(), 2);
    
    int last = nums.pop();
    ASSERT_EQ(last, 20);
    ASSERT_EQ(nums.len(), 1);
}

TEST(list_slice) {
    List<int> nums = {1, 2, 3, 4, 5};
    auto slice = nums.slice(1, 4);
    ASSERT_EQ(slice.len(), 3);
    ASSERT_EQ(slice[0], 2);
    ASSERT_EQ(slice[2], 4);
}

TEST(list_map_filter) {
    List<int> nums = {1, 2, 3, 4, 5};
    
    auto doubled = nums.map([](int x) { return x * 2; });
    ASSERT_EQ(doubled[0], 2);
    ASSERT_EQ(doubled[4], 10);
    
    auto evens = nums.filter([](int x) { return x % 2 == 0; });
    ASSERT_EQ(evens.len(), 2);
}

TEST(list_reduce) {
    List<int> nums = {1, 2, 3, 4, 5};
    int sum = nums.reduce(0, [](int acc, int x) { return acc + x; });
    ASSERT_EQ(sum, 15);
}

// === Dict Tests ===

TEST(dict_basic) {
    Dict<std::string, int> scores;
    scores["alice"] = 100;
    scores["bob"] = 95;
    
    ASSERT_EQ(scores.len(), 2);
    ASSERT_EQ(scores["alice"], 100);
    ASSERT_TRUE(scores.contains("bob"));
    ASSERT_TRUE(!scores.contains("charlie"));
}

TEST(dict_get_default) {
    Dict<std::string, int> scores;
    scores["alice"] = 100;
    
    ASSERT_EQ(scores.get("alice", 0), 100);
    ASSERT_EQ(scores.get("unknown", 50), 50);
}

TEST(dict_keys_values) {
    Dict<std::string, int> scores;
    scores["a"] = 1;
    scores["b"] = 2;
    
    auto keys = scores.keys();
    auto values = scores.values();
    
    ASSERT_EQ(keys.len(), 2);
    ASSERT_EQ(values.len(), 2);
}

// === Option Tests ===

TEST(option_some_none) {
    Option<int> some_val = Option<int>::some(42);
    Option<int> no_val = Option<int>::none();
    
    ASSERT_TRUE(some_val.is_some());
    ASSERT_TRUE(no_val.is_none());
    ASSERT_EQ(some_val.unwrap(), 42);
}

TEST(option_unwrap_or) {
    Option<int> none_val = Option<int>::none();
    ASSERT_EQ(none_val.unwrap_or(100), 100);
}

TEST(option_map) {
    Option<int> val = Option<int>::some(10);
    auto doubled = val.map([](int x) { return x * 2; });
    ASSERT_EQ(doubled.unwrap(), 20);
}

// === Result Tests ===

TEST(result_ok_err) {
    auto ok = Result<int, std::string>::ok(42);
    auto err = Result<int, std::string>::err("error");
    
    ASSERT_TRUE(ok.is_ok());
    ASSERT_TRUE(err.is_err());
    ASSERT_EQ(ok.unwrap(), 42);
}

TEST(result_unwrap_or) {
    auto err = Result<int, std::string>::err("error");
    ASSERT_EQ(err.unwrap_or(100), 100);
}

// === String Tests ===

TEST(string_basic) {
    String s = "Hello, World!";
    ASSERT_EQ(s.len(), 13);
    ASSERT_TRUE(s.contains("World"));
}

TEST(string_upper_lower) {
    String s = "Hello";
    ASSERT_EQ(s.upper().to_std(), "HELLO");
    ASSERT_EQ(s.lower().to_std(), "hello");
}

TEST(string_split_join) {
    String s = "a,b,c";
    auto parts = s.split(",");
    ASSERT_EQ(parts.len(), 3);
    ASSERT_EQ(parts[1].to_std(), "b");
    
    auto joined = String::join("-", parts);
    ASSERT_EQ(joined.to_std(), "a-b-c");
}

TEST(string_strip) {
    String s = "  hello  ";
    ASSERT_EQ(s.strip().to_std(), "hello");
}

// === Math Tests ===

TEST(math_constants) {
    ASSERT_TRUE(math::PI > 3.14 && math::PI < 3.15);
    ASSERT_TRUE(math::E > 2.71 && math::E < 2.72);
}

TEST(math_basic) {
    ASSERT_EQ(math::abs(static_cast<int64_t>(-5)), 5);
    ASSERT_EQ(math::floor(3.7), 3.0);
    ASSERT_EQ(math::ceil(3.2), 4.0);
    ASSERT_EQ(math::min(3, 5), 3);
    ASSERT_EQ(math::max(3, 5), 5);
}

TEST(math_trig) {
    ASSERT_TRUE(math::abs(math::sin(0.0)) < 0.0001);
    ASSERT_TRUE(math::abs(math::cos(0.0) - 1.0) < 0.0001);
}

TEST(math_pow_log) {
    ASSERT_EQ(math::pow(2, 3), 8.0);
    ASSERT_TRUE(math::abs(math::sqrt(4.0) - 2.0) < 0.0001);
    ASSERT_TRUE(math::abs(math::log(math::E) - 1.0) < 0.0001);
}

TEST(math_gcd_lcm) {
    ASSERT_EQ(math::gcd(12, 18), 6);
    ASSERT_EQ(math::lcm(4, 6), 12);
    ASSERT_EQ(math::factorial(5), 120);
}

TEST(math_random) {
    // Just test it runs without crashing
    double r = math::random();
    ASSERT_TRUE(r >= 0.0 && r < 1.0);
    
    int64_t ri = math::randint(1, 10);
    ASSERT_TRUE(ri >= 1 && ri <= 10);
}

TEST(math_statistics) {
    List<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
    ASSERT_EQ(math::sum(data), 15.0);
    ASSERT_EQ(math::mean(data), 3.0);
    ASSERT_EQ(math::median(data), 3.0);
}

// === IO Tests ===

TEST(io_format) {
    auto s = io::format("Hello, {}!", "World");
    ASSERT_EQ(s.to_std(), "Hello, World!");
    
    auto s2 = io::format("{} + {} = {}", 1, 2, 3);
    ASSERT_EQ(s2.to_std(), "1 + 2 = 3");
}

int main() {
    std::cout << "=== Axiom Stdlib Tests ===\n\n";
    
    // List
    RUN_TEST(list_basic);
    RUN_TEST(list_append_pop);
    RUN_TEST(list_slice);
    RUN_TEST(list_map_filter);
    RUN_TEST(list_reduce);
    
    // Dict
    RUN_TEST(dict_basic);
    RUN_TEST(dict_get_default);
    RUN_TEST(dict_keys_values);
    
    // Option
    RUN_TEST(option_some_none);
    RUN_TEST(option_unwrap_or);
    RUN_TEST(option_map);
    
    // Result
    RUN_TEST(result_ok_err);
    RUN_TEST(result_unwrap_or);
    
    // String
    RUN_TEST(string_basic);
    RUN_TEST(string_upper_lower);
    RUN_TEST(string_split_join);
    RUN_TEST(string_strip);
    
    // Math
    RUN_TEST(math_constants);
    RUN_TEST(math_basic);
    RUN_TEST(math_trig);
    RUN_TEST(math_pow_log);
    RUN_TEST(math_gcd_lcm);
    RUN_TEST(math_random);
    RUN_TEST(math_statistics);
    
    // IO
    RUN_TEST(io_format);
    
    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
