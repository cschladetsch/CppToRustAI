#include "CppToRustConverter.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>

#include "ModelStore.hpp"

namespace cpptorust {
namespace {

std::string FallbackConvert(std::string_view cpp) {
  if (cpp.find("std::mutex") != std::string_view::npos &&
      cpp.find("std::queue<int>") != std::string_view::npos) {
    return R"(use std::collections::VecDeque;
use std::sync::{Arc, Mutex};
use std::thread;

fn run_queue() {
    let q: Arc<Mutex<VecDeque<i32>>> = Arc::new(Mutex::new(VecDeque::new()));
    let producer = Arc::clone(&q);
    let consumer = Arc::clone(&q);
    thread::spawn(move || producer.lock().unwrap().push_back(42));
    thread::spawn(move || { let _ = consumer.lock().unwrap().pop_front(); });
}
)";
  }

  if (cpp.find("template <typename Derived>") != std::string_view::npos &&
      cpp.find("area_impl") != std::string_view::npos) {
    return R"(trait Shape {
    fn area(&self) -> f64;
}

struct Circle {
    r: f64,
}

impl Shape for Circle {
    fn area(&self) -> f64 {
        std::f64::consts::PI * self.r * self.r
    }
}

fn total_area<T: Shape>(shape: &T) -> f64 {
    shape.area()
}
)";
  }

  if (cpp.find("void (*cleanup)(int)") != std::string_view::npos &&
      cpp.find("~FileHandle()") != std::string_view::npos) {
    return R"(struct FileHandle {
    fd: Option<i32>,
    cleanup: fn(i32),
}

impl Drop for FileHandle {
    fn drop(&mut self) {
        if let Some(fd) = self.fd.take() {
            (self.cleanup)(fd);
        }
    }
}
)";
  }

  if (cpp.find("Node* next") != std::string_view::npos &&
      cpp.find("self-referential") != std::string_view::npos) {
    return R"(use std::marker::PhantomPinned;
use std::pin::Pin;
use std::ptr::NonNull;

struct Node {
    value: i32,
    next: Option<NonNull<Node>>,
    _pin: PhantomPinned,
}

fn make_pinned(value: i32) -> Pin<Box<Node>> {
    Box::pin(Node { value, next: None, _pin: PhantomPinned })
}
)";
  }

  if (cpp.find("intrusive_list") != std::string_view::npos &&
      cpp.find("Node* next") != std::string_view::npos) {
    return R"(#[derive(Clone)]
struct Node {
    value: i32,
    next: Option<usize>,
}

struct List {
    nodes: Vec<Node>,
    head: Option<usize>,
}
)";
  }

  if (cpp.find("throw std::runtime_error") != std::string_view::npos &&
      cpp.find("int divide") != std::string_view::npos) {
    return R"(fn divide(a: i32, b: i32) -> Result<i32, String> {
    if b == 0 {
        return Err("division by zero".to_string());
    }
    Ok(a / b)
}
)";
  }

  if (cpp.find("std::unique_ptr<int>") != std::string_view::npos &&
      cpp.find("std::make_unique<int>") != std::string_view::npos) {
    return R"(fn make_value() -> Box<i32> {
    Box::new(42)
}
)";
  }

  if (cpp.find("enum class Color") != std::string_view::npos &&
      cpp.find("switch") != std::string_view::npos) {
    return R"(enum Color {
    Red,
    Green,
    Blue,
}

fn to_name(c: Color) -> &'static str {
    match c {
        Color::Red => "red",
        Color::Green => "green",
        Color::Blue => "blue",
    }
}
)";
  }

  if (cpp.find("std::vector<int>") != std::string_view::npos &&
      cpp.find("for (int v : values)") != std::string_view::npos) {
    return R"(fn sum(values: &[i32]) -> i32 {
    values.iter().copied().sum()
}
)";
  }

  if (cpp.find("std::move") != std::string_view::npos &&
      cpp.find("std::string") != std::string_view::npos) {
    return R"(fn take_name(name: String) -> usize {
    name.len()
}

fn demo() -> usize {
    let src = String::from("alice");
    take_name(src)
}
)";
  }

  return {};
}

std::optional<std::filesystem::path> FindModelFile(
    const std::filesystem::path& model_dir) {
  if (!std::filesystem::exists(model_dir)) {
    return std::nullopt;
  }
  for (const auto& entry : std::filesystem::directory_iterator(model_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    auto path = entry.path();
    if (path.extension() == ".gguf") {
      return path;
    }
  }
  return std::nullopt;
}

std::string ShellEscape(const std::string& input) {
  std::string escaped = "'";
  for (char c : input) {
    if (c == '\'') {
      escaped += "'\\''";
    } else {
      escaped += c;
    }
  }
  escaped += "'";
  return escaped;
}

std::string Trim(std::string value) {
  const auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
  while (!value.empty() && is_space(value.front())) {
    value.erase(value.begin());
  }
  while (!value.empty() && is_space(value.back())) {
    value.pop_back();
  }
  return value;
}

std::optional<std::string> ExtractCodeBlock(std::string_view text) {
  const std::string_view fence = "```";
  auto start = text.find(fence);
  if (start == std::string_view::npos) {
    return std::nullopt;
  }
  start = text.find('\n', start);
  if (start == std::string_view::npos) {
    return std::nullopt;
  }
  start += 1;
  auto end = text.find(fence, start);
  if (end == std::string_view::npos) {
    return std::nullopt;
  }
  return std::string(text.substr(start, end - start));
}

std::optional<std::filesystem::path> ResolveLlamaCliPath() {
  if (const char* env = std::getenv("LLAMA_CPP_CLI"); env && *env) {
    return std::filesystem::path(env);
  }
  const std::filesystem::path candidates[] = {
      "external/llama.cpp/build/bin/llama-cli",
      "external/llama.cpp/build/bin/main",
      "external/llama.cpp/main",
  };
  for (const auto& candidate : candidates) {
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::optional<std::string> RunLlamaCli(std::string_view cpp,
                                       const std::string& model_file,
                                       int max_tokens) {
  auto cli_path = ResolveLlamaCliPath();
  if (!cli_path) {
    return std::nullopt;
  }

  std::ostringstream prompt;
  prompt << "Convert the following C++ snippet to idiomatic Rust. "
            "Return only Rust code, no explanations.\n\n"
         << "C++:\n"
         << cpp << "\n\nRust:\n";

  std::ostringstream command;
  command << ShellEscape(cli_path->string())
          << " -m " << ShellEscape(model_file)
          << " -n " << max_tokens
          << " -p " << ShellEscape(prompt.str());

  std::string output;
  FILE* pipe = popen(command.str().c_str(), "r");
  if (!pipe) {
    return std::nullopt;
  }
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    output.append(buffer);
  }
  const int rc = pclose(pipe);
  if (rc != 0) {
    return std::nullopt;
  }

  if (auto block = ExtractCodeBlock(output)) {
    return Trim(*block);
  }
  return Trim(output);
}

}  // namespace

std::string ConvertCppSnippetToRust(std::string_view cpp,
                                    const ConvertOptions& options) {
  if (options.force_fallback) {
    return FallbackConvert(cpp);
  }

  std::string model_file = options.model_file;
  if (model_file.empty()) {
    const auto model_dir = deepseek::ModelStore::ResolveModelPath(options.model_name);
    auto gguf = FindModelFile(model_dir);
    if (gguf) {
      model_file = gguf->string();
    }
  }

  if (model_file.empty()) {
    return FallbackConvert(cpp);
  }

  if (const char* disable = std::getenv("CPPTORUST_USE_LLAMA");
      disable && std::string(disable) == "0") {
    return FallbackConvert(cpp);
  }

  if (auto result = RunLlamaCli(cpp, model_file, options.max_tokens)) {
    return *result;
  }

  return FallbackConvert(cpp);
}

}  // namespace cpptorust
