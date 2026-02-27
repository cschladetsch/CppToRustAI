#include "DeepSeekStreamParser.hpp"

#include <iostream>
#include <string>
#include <vector>

int main() {
  std::vector<std::string> content_deltas;

  deepseek::DeepSeekStreamParser parser(
      [&](std::string_view /*reasoning_delta*/, std::string_view content_delta) {
        if (!content_delta.empty()) {
          content_deltas.emplace_back(content_delta);
          std::cout << "[content delta] " << content_delta << '\n';
        }
      });

  const std::vector<std::string> chunks = {
      "data: {\"choices\":[{\"delta\":{\"content\":\"Hel\"}}]}\n",
      "data: {\"choices\":[{\"delta\":{\"content\":\"lo",
      " world\"}}]}\n",
      "data: [DONE]\n",
  };

  std::string error;
  for (const auto& chunk : chunks) {
    if (!parser.Feed(chunk, &error)) {
      std::cerr << "Parse error: " << error << '\n';
      return 1;
    }
  }

  std::string final_text;
  for (const auto& d : content_deltas) {
    final_text += d;
  }

  std::cout << "Final content: " << final_text << '\n';
  return 0;
}
