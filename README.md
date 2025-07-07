# 🛠️ Hauss Programming Language

This project implements a compiler for a simple expression-based language. It includes:

- A tokenizer (`tokenization.hpp`)
- A recursive-descent parser with operator precedence
- An Abstract Syntax Tree (AST) based IR
- x86-64 assembly code generation
- Support for:
  - Arithmetic expressions (`+`, `-`, `*`, `/`)
  - Comparison operators (`<`, `>`, `<=`, `>=`, `==`)
  - Variable declarations and assignments
  - Conditionals (`if`, `elif`, `else`)
  - Blocks `{ ... }`
  - Built-in functions like `print(...)` and `exit(...)`

## 📦 Project Structure

```bash
.
├── main.cpp                # Entry point
├── tokenization.hpp        # Tokenizer and token types
├── parser.hpp              # AST nodes and parser logic
├── arena.hpp               # Simple bump allocator for AST memory
├── generation.hpp          # Code generator: turns AST into x86-64 assembly
└── README.md
```

## Compile and run

```bash
cmake --build build/
./build/hauss file.gs
./out
``

## Example

```
let x = -1;
if (x < 0){
    print(-1);
} else{
    print(1);
}
