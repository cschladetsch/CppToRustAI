#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
MODEL_CONFIG="${ROOT_DIR}/config/model_download.conf"

if [ -f "${MODEL_CONFIG}" ]; then
  # shellcheck disable=SC1090
  source "${MODEL_CONFIG}"
fi

is_submodule_present() {
  local path="$1"
  [ -f "${path}/.git" ] || [ -d "${path}/.git" ]
}

ensure_submodule() {
  local path="$1"
  local url="$2"
  if is_submodule_present "${path}"; then
    return 0
  fi
  if git -C "${ROOT_DIR}" config -f .gitmodules --get "submodule.${path#${ROOT_DIR}/}.path" >/dev/null 2>&1; then
    return 0
  fi
  git -C "${ROOT_DIR}" submodule add "${url}" "${path#${ROOT_DIR}/}"
}

declare -a REQUIRED_SUBMODULES=(
  "external/CppLmmModelStore|https://github.com/cschladetsch/CppLmmModelStore"
  "external/googletest|https://github.com/google/googletest.git"
  "external/llama.cpp|https://github.com/ggerganov/llama.cpp"
)

for entry in "${REQUIRED_SUBMODULES[@]}"; do
  IFS="|" read -r rel_path url <<< "${entry}"
  ensure_submodule "${ROOT_DIR}/${rel_path}" "${url}"
done

git -C "${ROOT_DIR}" submodule update --init --recursive
resolve_model_base() {
  if [ -n "${DEEPSEEK_MODEL_HOME:-}" ]; then
    echo "${DEEPSEEK_MODEL_HOME}"
    return 0
  fi
  if [ -n "${XDG_DATA_HOME:-}" ]; then
    echo "${XDG_DATA_HOME}/deepseek/models"
    return 0
  fi
  echo "${HOME}/.local/share/deepseek/models"
}

MODEL_BASE="$(resolve_model_base)"
MODEL_DIR="${MODEL_BASE}/${MODEL_NAME:-}"
MODEL_PATH="${MODEL_DIR}/${MODEL_FILE:-}"

if [ -n "${MODEL_NAME:-}" ] && [ -n "${MODEL_FILE:-}" ] && [ -n "${MODEL_URL:-}" ]; then
  mkdir -p "${MODEL_DIR}"
  if [ ! -f "${MODEL_PATH}" ]; then
    if command -v curl >/dev/null 2>&1; then
      curl -L -o "${MODEL_PATH}" "${MODEL_URL}"
    elif command -v wget >/dev/null 2>&1; then
      wget -O "${MODEL_PATH}" "${MODEL_URL}"
    else
      echo "Missing curl or wget to download ${MODEL_FILE}" >&2
      exit 1
    fi
  fi
fi

LLAMA_DIR="${ROOT_DIR}/external/llama.cpp"
LLAMA_BUILD_DIR="${LLAMA_DIR}/build"
LLAMA_CLI="${LLAMA_BUILD_DIR}/bin/llama-cli"

if [ -d "${LLAMA_DIR}" ] && [ ! -x "${LLAMA_CLI}" ]; then
  cmake -S "${LLAMA_DIR}" -B "${LLAMA_BUILD_DIR}"
  cmake --build "${LLAMA_BUILD_DIR}" -j
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j
