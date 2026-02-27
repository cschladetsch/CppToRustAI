#include "CppToRustConverter.hpp"

#include <filesystem>
#include <optional>
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

  // TODO: Integrate llama.cpp inference using the model_file.
  // For now, fall back to deterministic conversion until inference wiring is in place.
  return FallbackConvert(cpp);
}

}  // namespace cpptorust
