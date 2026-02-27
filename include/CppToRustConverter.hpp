#pragma once

#include <string>
#include <string_view>

namespace cpptorust {

struct ConvertOptions {
  // Model name as used by ModelStore, e.g. "deepseek-r1".
  std::string model_name = "deepseek-r1";
  // Optional explicit path to a GGUF model file. If empty, the converter
  // searches for a .gguf file inside the ModelStore directory for model_name.
  std::string model_file;
  // Max tokens to generate from the model.
  int max_tokens = 9999999;
  // Disable LLM usage and force the deterministic fallback converter.
  bool force_fallback = false;
};

// Converts a C++ snippet into Rust. Uses a local DeepSeek GGUF model when
// available; otherwise falls back to deterministic pattern-based conversion.
std::string ConvertCppSnippetToRust(std::string_view cpp,
                                    const ConvertOptions& options = {});

}  // namespace cpptorust
