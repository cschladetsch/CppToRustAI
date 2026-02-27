#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if [ -f "${ROOT_DIR}/.gitmodules" ]; then
  git -C "${ROOT_DIR}" submodule update --init --recursive
fi

if [ ! -e "${ROOT_DIR}/external/googletest" ]; then
  echo "Missing required submodule: external/googletest" >&2
  echo "Run: git submodule add https://github.com/google/googletest.git external/googletest" >&2
  echo "Then: git submodule update --init --recursive" >&2
  exit 1
fi
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j
