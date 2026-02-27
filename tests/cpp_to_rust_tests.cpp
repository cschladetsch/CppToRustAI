#include <gtest/gtest.h>
#include "CppToRustConverter.hpp"
#include <string>
#include <string_view>

namespace {

std::string ConvertCppSnippetToRust(std::string_view cpp) {
  cpptorust::ConvertOptions options;
  options.force_fallback = true;
  return cpptorust::ConvertCppSnippetToRust(cpp, options);
}

void ExpectContains(std::string_view output, std::string_view token,
                    std::string_view test_name) {
  ASSERT_NE(output.find(token), std::string_view::npos)
      << test_name << ": missing token '" << token << "'";
}

void TestSharedMutableStateConversion() {
  constexpr std::string_view kCpp = R"(std::mutex m;
std::queue<int> q;)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Arc<Mutex<VecDeque<i32>>>", "shared mutable state");
  ExpectContains(rust, "thread::spawn", "shared mutable state");
}

void TestSharedMutableStateConversionWithScope() {
  constexpr std::string_view kCpp = R"(void run() {
  std::mutex m;
  std::queue<int> q;
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "VecDeque<i32>", "shared mutable state (scope)");
  ExpectContains(rust, "Arc::new", "shared mutable state (scope)");
}

void TestSharedMutableStateConversionWithUsing() {
  constexpr std::string_view kCpp = R"(using std::mutex;
using std::queue;
std::mutex m;
std::queue<int> q;)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Mutex<VecDeque<i32>>", "shared mutable state (using)");
  ExpectContains(rust, "thread::spawn", "shared mutable state (using)");
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

void TestCrtpConversionWithImpl() {
  constexpr std::string_view kCpp = R"(template <typename Derived>
struct Shape {
  double area() const { return static_cast<const Derived*>(this)->area_impl(); }
};
struct Circle : Shape<Circle> { double area_impl() const; };)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "impl Shape for Circle", "crtp (impl)");
  ExpectContains(rust, "trait Shape", "crtp (impl)");
}

void TestCrtpConversionWithHelper() {
  constexpr std::string_view kCpp = R"(template <typename Derived>
struct Shape {
  double area() const { return static_cast<const Derived*>(this)->area_impl(); }
};
double total(const Shape<struct Circle>&);)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "fn total_area<T: Shape>", "crtp (helper)");
  ExpectContains(rust, "shape.area()", "crtp (helper)");
}

void TestTemplateCrptWithLambdaHelper() {
  constexpr std::string_view kCpp = R"(template <typename Derived>
struct Shape {
  double area() const { return static_cast<const Derived*>(this)->area_impl(); }
};
auto f = [](const Shape<struct Circle>&) { return 1.0; };)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "trait Shape", "crtp (lambda helper)");
  ExpectContains(rust, "fn total_area<T: Shape>", "crtp (lambda helper)");
}

void TestTemplateCrptWithLambdaBody() {
  constexpr std::string_view kCpp = R"(template <typename Derived>
struct Shape {
  double area() const { return static_cast<const Derived*>(this)->area_impl(); }
};
auto g = [](){ return 2.0; };)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "impl Shape for Circle", "crtp (lambda body)");
  ExpectContains(rust, "std::f64::consts::PI", "crtp (lambda body)");
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

void TestRaiiConversionWithCtor() {
  constexpr std::string_view kCpp = R"(struct FileHandle {
  int fd;
  void (*cleanup)(int);
  FileHandle(int f, void (*c)(int));
  ~FileHandle();
};)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "struct FileHandle", "raii (ctor)");
  ExpectContains(rust, "(self.cleanup)(fd)", "raii (ctor)");
}

void TestRaiiConversionWithNullableFd() {
  constexpr std::string_view kCpp = R"(struct FileHandle {
  int fd;
  void (*cleanup)(int);
  ~FileHandle();
  bool valid() const;
};)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Option<i32>", "raii (nullable)");
  ExpectContains(rust, "take()", "raii (nullable)");
}

void TestRaiiConversionWithLambda() {
  constexpr std::string_view kCpp = R"(struct FileHandle {
  int fd;
  void (*cleanup)(int);
  ~FileHandle();
};
auto closer = [](int fd){ (void)fd; };)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "impl Drop for FileHandle", "raii (lambda)");
  ExpectContains(rust, "cleanup: fn(i32)", "raii (lambda)");
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

void TestPinnedSelfReferenceConversionWithCtor() {
  constexpr std::string_view kCpp = R"(// self-referential
struct Node {
  int value;
  Node* next;
  Node(int v) : value(v), next(nullptr) {}
};)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Box::pin", "self-referential (ctor)");
  ExpectContains(rust, "PhantomPinned", "self-referential (ctor)");
}

void TestPinnedSelfReferenceConversionWithHelper() {
  constexpr std::string_view kCpp = R"(// self-referential
struct Node {
  int value;
  Node* next;
};
Node* make_node();)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Pin<Box<Node>>", "self-referential (helper)");
  ExpectContains(rust, "NonNull<Node>", "self-referential (helper)");
}

void TestPinnedSelfReferenceConversionWithLambda() {
  constexpr std::string_view kCpp = R"(// self-referential
struct Node {
  int value;
  Node* next;
};
auto make = [](){ return Node{}; };)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "PhantomPinned", "self-referential (lambda)");
  ExpectContains(rust, "NonNull<Node>", "self-referential (lambda)");
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

void TestIndexBasedIntrusiveListConversionWithStruct() {
  constexpr std::string_view kCpp = R"(// intrusive_list
struct Node {
  int value;
  Node* next;
};
struct List { Node* head; };)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "struct List", "intrusive list (struct)");
  ExpectContains(rust, "head: Option<usize>", "intrusive list (struct)");
}

void TestIndexBasedIntrusiveListConversionWithCtor() {
  constexpr std::string_view kCpp = R"(// intrusive_list
struct Node {
  int value;
  Node* next;
  Node(int v) : value(v), next(nullptr) {}
};)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "nodes: Vec<Node>", "intrusive list (ctor)");
  ExpectContains(rust, "Option<usize>", "intrusive list (ctor)");
}

void TestIndexBasedIntrusiveListConversionWithLambda() {
  constexpr std::string_view kCpp = R"(// intrusive_list
struct Node {
  int value;
  Node* next;
};
auto build = [](Node* head){ return head; };)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "struct List", "intrusive list (lambda)");
  ExpectContains(rust, "head: Option<usize>", "intrusive list (lambda)");
}

void TestExceptionToResultConversion() {
  constexpr std::string_view kCpp = R"(int divide(int a, int b) {
  if (b == 0) throw std::runtime_error("division by zero");
  return a / b;
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Result<i32, String>", "exception->result");
  ExpectContains(rust, "Err(", "exception->result");
}

void TestExceptionToResultConversionWithElse() {
  constexpr std::string_view kCpp = R"(int divide(int a, int b) {
  if (b == 0) throw std::runtime_error("division by zero");
  else return a / b;
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Result<i32, String>", "exception->result (else)");
  ExpectContains(rust, "Ok(a / b)", "exception->result (else)");
}

void TestExceptionToResultConversionWithGuard() {
  constexpr std::string_view kCpp = R"(int divide(int a, int b) {
  if (b == 0) throw std::runtime_error("division by zero");
  return a / b;
}
int safe(int a, int b) { return divide(a, b); })";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Err(\"division by zero\".to_string())",
                 "exception->result (guard)");
  ExpectContains(rust, "Ok(a / b)", "exception->result (guard)");
}

void TestExceptionToResultConversionWithLambda() {
  constexpr std::string_view kCpp = R"(int divide(int a, int b) {
  if (b == 0) throw std::runtime_error("division by zero");
  return a / b;
}
auto f = [](int a, int b){ return divide(a, b); };)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Result<i32, String>", "exception->result (lambda)");
  ExpectContains(rust, "Err(\"division by zero\".to_string())",
                 "exception->result (lambda)");
}

void TestUniquePtrToBoxConversion() {
  constexpr std::string_view kCpp = R"(std::unique_ptr<int> make_value() {
  return std::make_unique<int>(42);
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Box<i32>", "unique_ptr->box");
  ExpectContains(rust, "Box::new(42)", "unique_ptr->box");
}

void TestUniquePtrToBoxConversionWithLocal() {
  constexpr std::string_view kCpp = R"(std::unique_ptr<int> make_value() {
  auto v = std::make_unique<int>(42);
  return v;
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Box<i32>", "unique_ptr->box (local)");
  ExpectContains(rust, "Box::new(42)", "unique_ptr->box (local)");
}

void TestUniquePtrToBoxConversionWithTypedef() {
  constexpr std::string_view kCpp = R"(using IntPtr = std::unique_ptr<int>;
IntPtr make_value() { return std::make_unique<int>(42); })";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "fn make_value()", "unique_ptr->box (typedef)");
  ExpectContains(rust, "Box<i32>", "unique_ptr->box (typedef)");
}

void TestUniquePtrToBoxConversionWithTemplate() {
  constexpr std::string_view kCpp =
      R"(template <typename T>
std::unique_ptr<int> make_value() { return std::make_unique<int>(42); })";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Box<i32>", "unique_ptr->box (template)");
  ExpectContains(rust, "Box::new(42)", "unique_ptr->box (template)");
}

void TestUniquePtrToBoxConversionWithLambda() {
  constexpr std::string_view kCpp = R"(std::unique_ptr<int> make_value() {
  auto f = [](){ return 42; };
  return std::make_unique<int>(f());
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "fn make_value()", "unique_ptr->box (lambda)");
  ExpectContains(rust, "Box::new(42)", "unique_ptr->box (lambda)");
}

void TestEnumClassToRustEnumConversion() {
  constexpr std::string_view kCpp = R"(enum class Color { Red, Green, Blue };
const char* to_name(Color c) {
  switch (c) { default: return "red"; }
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "enum Color", "enum class");
  ExpectContains(rust, "match c", "enum class");
}

void TestEnumClassToRustEnumConversionWithCase() {
  constexpr std::string_view kCpp = R"(enum class Color { Red, Green, Blue };
const char* to_name(Color c) {
  switch (c) { case Color::Red: return "red"; default: return "blue"; }
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "Color::Red", "enum class (case)");
  ExpectContains(rust, "match c", "enum class (case)");
}

void TestEnumClassToRustEnumConversionWithFn() {
  constexpr std::string_view kCpp = R"(enum class Color { Red, Green, Blue };
void log(Color c);
const char* to_name(Color c) { switch (c) { default: return "red"; } })";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "enum Color", "enum class (fn)");
  ExpectContains(rust, "fn to_name", "enum class (fn)");
}

void TestEnumClassToRustEnumConversionWithLambda() {
  constexpr std::string_view kCpp = R"(enum class Color { Red, Green, Blue };
auto f = [](Color c) { return c; };
const char* to_name(Color c) { switch (c) { default: return "red"; } })";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "match c", "enum class (lambda)");
  ExpectContains(rust, "Color::Blue", "enum class (lambda)");
}

void TestVectorLoopToIteratorConversion() {
  constexpr std::string_view kCpp = R"(int sum(const std::vector<int>& values) {
  int total = 0;
  for (int v : values) total += v;
  return total;
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "values.iter().copied().sum()", "vector loop");
  ExpectContains(rust, "&[i32]", "vector loop");
}

void TestVectorLoopToIteratorConversionWithBraces() {
  constexpr std::string_view kCpp = R"(int sum(const std::vector<int>& values) {
  int total = 0;
  for (int v : values) { total += v; }
  return total;
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "iter().copied().sum()", "vector loop (braces)");
  ExpectContains(rust, "fn sum(values: &[i32])", "vector loop (braces)");
}

void TestVectorLoopToIteratorConversionWithConst() {
  constexpr std::string_view kCpp = R"(int sum(const std::vector<int>& values) {
  const int* p = values.data();
  for (int v : values) { (void)p; }
  return 0;
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "values.iter().copied()", "vector loop (const)");
  ExpectContains(rust, "-> i32", "vector loop (const)");
}

void TestVectorLoopToIteratorConversionWithLambda() {
  constexpr std::string_view kCpp = R"(int sum(const std::vector<int>& values) {
  auto f = [](int v){ return v; };
  int total = 0;
  for (int v : values) total += f(v);
  return total;
})";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "values.iter().copied().sum()", "vector loop (lambda)");
  ExpectContains(rust, "&[i32]", "vector loop (lambda)");
}

void TestMoveSemanticsToOwnershipConversion() {
  constexpr std::string_view kCpp = R"(std::string src = "alice";
auto n = consume(std::move(src));)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "fn take_name(name: String)", "move semantics");
  ExpectContains(rust, "take_name(src)", "move semantics");
}

void TestMoveSemanticsToOwnershipConversionWithReturn() {
  constexpr std::string_view kCpp = R"(std::string src = "alice";
size_t n = consume(std::move(src));
return n;)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "fn demo() -> usize", "move semantics (return)");
  ExpectContains(rust, "take_name(src)", "move semantics (return)");
}

void TestMoveSemanticsToOwnershipConversionWithScope() {
  constexpr std::string_view kCpp = R"(std::string src = "alice";
{ auto n = consume(std::move(src)); })";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "let src = String::from(\"alice\")", "move semantics (scope)");
  ExpectContains(rust, "take_name(src)", "move semantics (scope)");
}

void TestMoveSemanticsToOwnershipConversionWithLambda() {
  constexpr std::string_view kCpp = R"(std::string src = "alice";
auto f = [](std::string s){ return s.size(); };
auto n = f(std::move(src));)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "fn take_name(name: String)", "move semantics (lambda)");
  ExpectContains(rust, "take_name(src)", "move semantics (lambda)");
}

void TestMoveSemanticsToOwnershipConversionWithTemplate() {
  constexpr std::string_view kCpp = R"(template <typename T>
T consume(T value);
std::string src = "alice";
auto n = consume(std::move(src));)";
  const std::string rust = ConvertCppSnippetToRust(kCpp);
  ExpectContains(rust, "fn demo() -> usize", "move semantics (template)");
  ExpectContains(rust, "take_name(src)", "move semantics (template)");
}

}  // namespace

TEST(CppToRustTests, SharedMutableStateConversion) {
  TestSharedMutableStateConversion();
}

TEST(CppToRustTests, SharedMutableStateConversionWithScope) {
  TestSharedMutableStateConversionWithScope();
}

TEST(CppToRustTests, SharedMutableStateConversionWithUsing) {
  TestSharedMutableStateConversionWithUsing();
}

TEST(CppToRustTests, CrtpConversion) { TestCrtpConversion(); }

TEST(CppToRustTests, CrtpConversionWithImpl) { TestCrtpConversionWithImpl(); }

TEST(CppToRustTests, CrtpConversionWithHelper) {
  TestCrtpConversionWithHelper();
}

TEST(CppToRustTests, TemplateCrptWithLambdaHelper) {
  TestTemplateCrptWithLambdaHelper();
}

TEST(CppToRustTests, TemplateCrptWithLambdaBody) {
  TestTemplateCrptWithLambdaBody();
}

TEST(CppToRustTests, RaiiConversion) { TestRaiiConversion(); }

TEST(CppToRustTests, RaiiConversionWithCtor) {
  TestRaiiConversionWithCtor();
}

TEST(CppToRustTests, RaiiConversionWithNullableFd) {
  TestRaiiConversionWithNullableFd();
}

TEST(CppToRustTests, RaiiConversionWithLambda) {
  TestRaiiConversionWithLambda();
}

TEST(CppToRustTests, PinnedSelfReferenceConversion) {
  TestPinnedSelfReferenceConversion();
}

TEST(CppToRustTests, PinnedSelfReferenceConversionWithCtor) {
  TestPinnedSelfReferenceConversionWithCtor();
}

TEST(CppToRustTests, PinnedSelfReferenceConversionWithHelper) {
  TestPinnedSelfReferenceConversionWithHelper();
}

TEST(CppToRustTests, PinnedSelfReferenceConversionWithLambda) {
  TestPinnedSelfReferenceConversionWithLambda();
}

TEST(CppToRustTests, IndexBasedIntrusiveListConversion) {
  TestIndexBasedIntrusiveListConversion();
}

TEST(CppToRustTests, IndexBasedIntrusiveListConversionWithStruct) {
  TestIndexBasedIntrusiveListConversionWithStruct();
}

TEST(CppToRustTests, IndexBasedIntrusiveListConversionWithCtor) {
  TestIndexBasedIntrusiveListConversionWithCtor();
}

TEST(CppToRustTests, IndexBasedIntrusiveListConversionWithLambda) {
  TestIndexBasedIntrusiveListConversionWithLambda();
}

TEST(CppToRustTests, ExceptionToResultConversion) {
  TestExceptionToResultConversion();
}

TEST(CppToRustTests, ExceptionToResultConversionWithElse) {
  TestExceptionToResultConversionWithElse();
}

TEST(CppToRustTests, ExceptionToResultConversionWithGuard) {
  TestExceptionToResultConversionWithGuard();
}

TEST(CppToRustTests, ExceptionToResultConversionWithLambda) {
  TestExceptionToResultConversionWithLambda();
}

TEST(CppToRustTests, UniquePtrToBoxConversion) {
  TestUniquePtrToBoxConversion();
}

TEST(CppToRustTests, UniquePtrToBoxConversionWithLocal) {
  TestUniquePtrToBoxConversionWithLocal();
}

TEST(CppToRustTests, UniquePtrToBoxConversionWithTypedef) {
  TestUniquePtrToBoxConversionWithTypedef();
}

TEST(CppToRustTests, UniquePtrToBoxConversionWithTemplate) {
  TestUniquePtrToBoxConversionWithTemplate();
}

TEST(CppToRustTests, UniquePtrToBoxConversionWithLambda) {
  TestUniquePtrToBoxConversionWithLambda();
}

TEST(CppToRustTests, EnumClassToRustEnumConversion) {
  TestEnumClassToRustEnumConversion();
}

TEST(CppToRustTests, EnumClassToRustEnumConversionWithCase) {
  TestEnumClassToRustEnumConversionWithCase();
}

TEST(CppToRustTests, EnumClassToRustEnumConversionWithFn) {
  TestEnumClassToRustEnumConversionWithFn();
}

TEST(CppToRustTests, EnumClassToRustEnumConversionWithLambda) {
  TestEnumClassToRustEnumConversionWithLambda();
}

TEST(CppToRustTests, VectorLoopToIteratorConversion) {
  TestVectorLoopToIteratorConversion();
}

TEST(CppToRustTests, VectorLoopToIteratorConversionWithBraces) {
  TestVectorLoopToIteratorConversionWithBraces();
}

TEST(CppToRustTests, VectorLoopToIteratorConversionWithConst) {
  TestVectorLoopToIteratorConversionWithConst();
}

TEST(CppToRustTests, VectorLoopToIteratorConversionWithLambda) {
  TestVectorLoopToIteratorConversionWithLambda();
}

TEST(CppToRustTests, MoveSemanticsToOwnershipConversion) {
  TestMoveSemanticsToOwnershipConversion();
}

TEST(CppToRustTests, MoveSemanticsToOwnershipConversionWithReturn) {
  TestMoveSemanticsToOwnershipConversionWithReturn();
}

TEST(CppToRustTests, MoveSemanticsToOwnershipConversionWithScope) {
  TestMoveSemanticsToOwnershipConversionWithScope();
}

TEST(CppToRustTests, MoveSemanticsToOwnershipConversionWithLambda) {
  TestMoveSemanticsToOwnershipConversionWithLambda();
}

TEST(CppToRustTests, MoveSemanticsToOwnershipConversionWithTemplate) {
  TestMoveSemanticsToOwnershipConversionWithTemplate();
}
