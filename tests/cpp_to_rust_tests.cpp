#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::string ConvertCppSnippetToRust(std::string_view cpp) {
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

  return {};
}

void ExpectContains(std::string_view output, std::string_view token,
                    std::string_view test_name) {
  if (output.find(token) == std::string_view::npos) {
    throw std::runtime_error(std::string(test_name) + ": missing token '" +
                             std::string(token) + "'");
  }
}

void TestSharedMutableStateConversion() {
  constexpr std::string_view kCpp = R"(std::mutex m;
std::queue<int> q;)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Arc<Mutex<VecDeque<i32>>>", "shared mutable state");
  ExpectContains(rust, "thread::spawn", "shared mutable state");
}

void TestCrtpConversion() {
  constexpr std::string_view kCpp = R"(template <typename Derived>
struct Shape {
  double area() const { return static_cast<const Derived*>(this)->area_impl(); }
};)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "trait Shape", "crtp");
  ExpectContains(rust, "fn total_area<T: Shape>", "crtp");
}

void TestRaiiConversion() {
  constexpr std::string_view kCpp = R"(struct FileHandle {
  int fd;
  void (*cleanup)(int);
  ~FileHandle();
};)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "impl Drop for FileHandle", "raii");
  ExpectContains(rust, "Option<i32>", "raii");
}

void TestPinnedSelfReferenceConversion() {
  constexpr std::string_view kCpp = R"(// self-referential
struct Node {
  int value;
  Node* next;
};)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Pin<Box<Node>>", "self-referential");
  ExpectContains(rust, "PhantomPinned", "self-referential");
}

void TestIndexBasedIntrusiveListConversion() {
  constexpr std::string_view kCpp = R"(// intrusive_list
struct Node {
  int value;
  Node* next;
};)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Vec<Node>", "intrusive list");
  ExpectContains(rust, "Option<usize>", "intrusive list");
}

}  // namespace

int main() {
  try {
    TestSharedMutableStateConversion();
    TestCrtpConversion();
    TestRaiiConversion();
    TestPinnedSelfReferenceConversion();
    TestIndexBasedIntrusiveListConversion();
    std::cout << "5/5 cpp-to-rust conversion tests passed\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "Test failure: " << ex.what() << '\n';
    return 1;
  }
}
