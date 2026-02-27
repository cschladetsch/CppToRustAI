#include "ModelStore.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  const std::string model = (argc > 1) ? argv[1] : "deepseek-r1";
  std::string error;
  auto created = deepseek::ModelStore::EnsureModelDir(model, &error);

  if (!created.has_value()) {
    std::cerr << "Failed: " << error << '\n';
    return 1;
  }

  std::cout << "Ensured model directory: " << *created << '\n';
  return 0;
}
