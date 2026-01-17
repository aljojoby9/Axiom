# Axiom Programming Language

<p align="center">
  <strong>A high-performance, Pythonic language for Data Science</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-0.1.0-blue" alt="Version">
  <img src="https://img.shields.io/badge/C%2B%2B-20-orange" alt="C++20">
  <img src="https://img.shields.io/badge/license-MIT-green" alt="License">
</p>

---

## ✨ Features

- 🚀 **Blazing Fast** — Compiled to native code via LLVM
- 🐍 **Pythonic Syntax** — Familiar to Python developers
- 🔒 **Type Safe** — Static typing with powerful inference
- 📊 **Data Science First** — First-class tensors and vectorized operations
- ⚡ **True Parallelism** — No GIL, async/await, parallel primitives
- 🎯 **Hybrid Paradigm** — OOP + Functional + Procedural

## 📝 Quick Example

```python
fn main():
    let data: Tensor[f64] = Tensor.randn((100, 10))
    let model = LinearRegression()
    model.fit(data[:, :-1], data[:, -1])
    print(f"Score: {model.score()}")
```

## 🛠️ Building from Source

### Prerequisites

- CMake 3.20+
- C++20 compiler (GCC 12+, Clang 15+, or MSVC 2022)
- LLVM 17+ (optional, for code generation)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/yourusername/axiom.git
cd axiom

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
ctest --test-dir build

# Run the compiler
./build/bin/axiom examples/hello_world.ax
```

## 📚 Documentation

- [Language Specification](docs/language_spec.md)
- [Tutorial](docs/tutorial.md)
- [Standard Library Reference](docs/stdlib_reference.md)

## 🗺️ Roadmap

- [x] Phase 1: Lexer
- [ ] Phase 2: Parser
- [ ] Phase 3: Semantic Analysis
- [ ] Phase 4: LLVM Code Generation
- [ ] Phase 5: Standard Library
- [ ] Phase 6: Tooling (REPL, Package Manager)

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.
