#pragma once

/**
 * @file math.hpp
 * @brief Math module for Axiom standard library
 * 
 * Provides mathematical functions and constants.
 */

#include <cmath>
#include <limits>
#include <random>
#include <algorithm>

namespace axiom {
namespace stdlib {
namespace math {

// Mathematical constants
constexpr double PI = 3.14159265358979323846;
constexpr double E = 2.71828182845904523536;
constexpr double TAU = 2.0 * PI;
constexpr double PHI = 1.61803398874989484820;  // Golden ratio
constexpr double INF = std::numeric_limits<double>::infinity();
constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();

// === Basic Functions ===

inline double abs(double x) { return std::abs(x); }
inline int64_t abs(int64_t x) { return std::abs(x); }

inline double floor(double x) { return std::floor(x); }
inline double ceil(double x) { return std::ceil(x); }
inline double round(double x) { return std::round(x); }
inline double trunc(double x) { return std::trunc(x); }

template<typename T>
inline T min(T a, T b) { return std::min(a, b); }

template<typename T>
inline T max(T a, T b) { return std::max(a, b); }

template<typename T>
inline T clamp(T value, T low, T high) {
    return std::clamp(value, low, high);
}

inline int64_t sign(double x) {
    return (x > 0) - (x < 0);
}

// === Power and Logarithm ===

inline double pow(double base, double exp) { return std::pow(base, exp); }
inline double sqrt(double x) { return std::sqrt(x); }
inline double cbrt(double x) { return std::cbrt(x); }
inline double hypot(double x, double y) { return std::hypot(x, y); }

inline double exp(double x) { return std::exp(x); }
inline double exp2(double x) { return std::exp2(x); }
inline double expm1(double x) { return std::expm1(x); }

inline double log(double x) { return std::log(x); }
inline double log2(double x) { return std::log2(x); }
inline double log10(double x) { return std::log10(x); }
inline double log1p(double x) { return std::log1p(x); }

// === Trigonometric Functions ===

inline double sin(double x) { return std::sin(x); }
inline double cos(double x) { return std::cos(x); }
inline double tan(double x) { return std::tan(x); }

inline double asin(double x) { return std::asin(x); }
inline double acos(double x) { return std::acos(x); }
inline double atan(double x) { return std::atan(x); }
inline double atan2(double y, double x) { return std::atan2(y, x); }

// Hyperbolic functions
inline double sinh(double x) { return std::sinh(x); }
inline double cosh(double x) { return std::cosh(x); }
inline double tanh(double x) { return std::tanh(x); }

inline double asinh(double x) { return std::asinh(x); }
inline double acosh(double x) { return std::acosh(x); }
inline double atanh(double x) { return std::atanh(x); }

// Degree/Radian conversion
inline double radians(double degrees) { return degrees * PI / 180.0; }
inline double degrees(double radians) { return radians * 180.0 / PI; }

// === Special Functions ===

inline double fmod(double x, double y) { return std::fmod(x, y); }
inline double copysign(double x, double y) { return std::copysign(x, y); }

inline bool isnan(double x) { return std::isnan(x); }
inline bool isinf(double x) { return std::isinf(x); }
inline bool isfinite(double x) { return std::isfinite(x); }

// Gamma and factorial
inline double gamma(double x) { return std::tgamma(x); }
inline double lgamma(double x) { return std::lgamma(x); }

inline int64_t factorial(int64_t n) {
    if (n <= 1) return 1;
    int64_t result = 1;
    for (int64_t i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

// GCD and LCM
inline int64_t gcd(int64_t a, int64_t b) {
    while (b != 0) {
        int64_t t = b;
        b = a % b;
        a = t;
    }
    return std::abs(a);
}

inline int64_t lcm(int64_t a, int64_t b) {
    return (a / gcd(a, b)) * b;
}

// === Random Number Generation ===

class Random {
public:
    Random() : gen_(std::random_device{}()) {}
    Random(uint64_t seed) : gen_(seed) {}
    
    void seed(uint64_t s) { gen_.seed(s); }
    
    // Uniform random double in [0, 1)
    double random() {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(gen_);
    }
    
    // Uniform random double in [a, b)
    double uniform(double a, double b) {
        std::uniform_real_distribution<double> dist(a, b);
        return dist(gen_);
    }
    
    // Uniform random int in [a, b]
    int64_t randint(int64_t a, int64_t b) {
        std::uniform_int_distribution<int64_t> dist(a, b);
        return dist(gen_);
    }
    
    // Random choice from list
    template<typename T>
    const T& choice(const List<T>& items) {
        return items[randint(0, items.len() - 1)];
    }
    
    // Shuffle list in place
    template<typename T>
    void shuffle(List<T>& items) {
        auto& vec = items.to_vector();
        std::shuffle(vec.begin(), vec.end(), gen_);
    }
    
    // Normal distribution
    double gauss(double mean = 0.0, double stddev = 1.0) {
        std::normal_distribution<double> dist(mean, stddev);
        return dist(gen_);
    }
    
    // Exponential distribution
    double exponential(double lambda = 1.0) {
        std::exponential_distribution<double> dist(lambda);
        return dist(gen_);
    }

private:
    std::mt19937_64 gen_;
};

// Global random instance
inline Random& default_random() {
    static Random rng;
    return rng;
}

inline double random() { return default_random().random(); }
inline double uniform(double a, double b) { return default_random().uniform(a, b); }
inline int64_t randint(int64_t a, int64_t b) { return default_random().randint(a, b); }

template<typename T>
inline const T& choice(const List<T>& items) { 
    return default_random().choice(items); 
}

template<typename T>
inline void shuffle(List<T>& items) { 
    default_random().shuffle(items); 
}

// === Statistical Functions ===

template<typename T>
T sum(const List<T>& items) {
    T result{};
    for (int64_t i = 0; i < items.len(); ++i) {
        result += items[i];
    }
    return result;
}

template<typename T>
double mean(const List<T>& items) {
    if (items.empty()) return 0.0;
    return static_cast<double>(sum(items)) / items.len();
}

template<typename T>
double variance(const List<T>& items) {
    if (items.len() < 2) return 0.0;
    double m = mean(items);
    double var = 0.0;
    for (int64_t i = 0; i < items.len(); ++i) {
        double diff = items[i] - m;
        var += diff * diff;
    }
    return var / (items.len() - 1);
}

template<typename T>
double stddev(const List<T>& items) {
    return sqrt(variance(items));
}

template<typename T>
T median(List<T> items) {
    if (items.empty()) return T{};
    items.sort();
    int64_t mid = items.len() / 2;
    if (items.len() % 2 == 0) {
        return (items[mid - 1] + items[mid]) / 2;
    }
    return items[mid];
}

} // namespace math
} // namespace stdlib
} // namespace axiom
