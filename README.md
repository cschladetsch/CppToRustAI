# CppToRust

Companion project for the article on AI-assisted C++ to Rust migration.

```mermaid
flowchart TD
  A[CppToRust Root] --> B[external/CppLmmModelStore submodule]
  A --> B2[external/googletest submodule]
  A --> C[demos]
  A --> D[tests]
  E[./b or CMake configure] --> F[Build ModelStore library]
  F --> G[Build 5 demos]
  F --> H[Build cpp_to_rust_tests]
  H --> I[./t or ctest]
```

## Included

- `external/CppLmmModelStore` git submodule
- `external/googletest` git submodule
- 5 demos in `demos/`
- 40 C++->Rust conversion tests in `tests/cpp_to_rust_tests.cpp` (GoogleTest)

## Setup

```bash
git submodule update --init --recursive
```

## Build

```bash
./b
```

## Demo Execution

```bash
./build/demo_01_resolve_paths deepseek-r1
DEEPSEEK_MODEL_HOME=/tmp/deepseek_models_demo ./build/demo_02_ensure_model_dir deepseek-r1
DEEPSEEK_MODEL_HOME=/tmp/deepseek_models_demo ./build/demo_03_model_exists deepseek-r1
./build/demo_04_stream_parser_basic
./build/demo_05_stream_parser_chunked
```

```mermaid
sequenceDiagram
  participant U as Operator
  participant D as Demo Binary
  participant M as ModelStore
  participant FS as Filesystem
  U->>D: execute demo_02/demo_03
  D->>M: EnsureModelDir/ModelExists
  M->>FS: create/check model path
  FS-->>M: status
  M-->>D: result
  D-->>U: output
```

Notes:
- `ModelStore` default location is `~/.local/share/deepseek/models`.
- `DEEPSEEK_MODEL_HOME=/tmp/...` in demo commands is only for restricted sandbox runs.

## Test Execution

```bash
./t
```
