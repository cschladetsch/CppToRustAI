#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

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
)

for entry in "${REQUIRED_SUBMODULES[@]}"; do
  IFS="|" read -r rel_path url <<< "${entry}"
  ensure_submodule "${ROOT_DIR}/${rel_path}" "${url}"
done

git -C "${ROOT_DIR}" submodule update --init --recursive
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j
