#include "DeepSeekStreamParser.hpp"

#include <iostream>
#include <string>

int main() {
  std::string reasoning_all;
  std::string content_all;

  deepseek::DeepSeekStreamParser parser(
      [&](std::string_view reasoning_delta, std::string_view content_delta) {
        if (!reasoning_delta.empty()) {
          reasoning_all += std::string(reasoning_delta);
          std::cout << "[reasoning delta] " << reasoning_delta << '\n';
        }
        if (!content_delta.empty()) {
          content_all += std::string(content_delta);
          std::cout << "[content delta] " << content_delta << '\n';
        }
      });

  const std::string stream =
      "data: {\"choices\":[{\"delta\":{\"reasoning_content\":\"Plan:\"}}]}\n"
      "data: {\"choices\":[{\"delta\":{\"content\":\"Hello\"}}]}\n"
      "data: {\"choices\":[{\"delta\":{\"content\":\" world\"}}]}\n"
      "data: [DONE]\n";

  std::string error;
  if (!parser.Feed(stream, &error)) {
    std::cerr << "Parse error: " << error << '\n';
    return 1;
  }

  std::cout << "Reasoning aggregate: " << reasoning_all << '\n';
  std::cout << "Content aggregate: " << content_all << '\n';
  return 0;
}
