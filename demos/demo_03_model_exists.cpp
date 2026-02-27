#include "ModelStore.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  const std::string model = (argc > 1) ? argv[1] : "deepseek-r1";

  std::cout << "Before ensure, exists? "
            << (deepseek::ModelStore::ModelExists(model) ? "yes" : "no") << '\n';

  std::string error;
  auto created = deepseek::ModelStore::EnsureModelDir(model, &error);
  if (!created.has_value()) {
    std::cerr << "Ensure failed: " << error << '\n';
    return 1;
  }

  std::cout << "After ensure, exists? "
            << (deepseek::ModelStore::ModelExists(model) ? "yes" : "no") << '\n';
  std::cout << "Path: " << *created << '\n';
  return 0;
}
