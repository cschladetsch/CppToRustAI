#include "ModelStore.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  const std::string model = (argc > 1) ? argv[1] : "deepseek-r1";

  std::cout << "Model home: " << deepseek::ModelStore::ResolveModelHome() << '\n';
  std::cout << "Model path (" << model << "): "
            << deepseek::ModelStore::ResolveModelPath(model) << '\n';
  return 0;
}
