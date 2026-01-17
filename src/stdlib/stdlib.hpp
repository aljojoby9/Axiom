#pragma once

/**
 * @file stdlib.hpp
 * @brief Main header for Axiom standard library
 * 
 * Include this to get all standard library modules.
 */

#include "core.hpp"
#include "io.hpp"
#include "math.hpp"

namespace axiom {
namespace stdlib {

// Bring common types into stdlib namespace for convenience
using axiom::stdlib::List;
using axiom::stdlib::Dict;
using axiom::stdlib::Option;
using axiom::stdlib::Result;
using axiom::stdlib::String;

} // namespace stdlib
} // namespace axiom
