#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

ensure_submodule() {
  local path="$1"
  local url="$2"
  if [ -d "${path}/.git" ] || git -C "${ROOT_DIR}" config -f .gitmodules --get "submodule.${path#${ROOT_DIR}/}.path" >/dev/null 2>&1; then
    return 0
  fi
  git -C "${ROOT_DIR}" submodule add "${url}" "${path#${ROOT_DIR}/}"
}

if [ -f "${ROOT_DIR}/.gitmodules" ]; then
  while IFS= read -r line; do
    key="${line%% *}"
    value="${line#* }"
    if [[ "${key}" == submodule.*.path ]]; then
      name="${key#submodule.}"
      name="${name%.path}"
      path="${value}"
      url="$(git -C "${ROOT_DIR}" config -f .gitmodules --get "submodule.${name}.url" || true)"
      if [ -n "${url}" ]; then
        ensure_submodule "${ROOT_DIR}/${path}" "${url}"
      fi
    fi
  done < <(git -C "${ROOT_DIR}" config -f .gitmodules --list | awk '{print $1, $2}')
fi

if [ ! -f "${ROOT_DIR}/.gitmodules" ] || \
   ! git -C "${ROOT_DIR}" config -f .gitmodules --get "submodule.external/googletest.url" >/dev/null 2>&1; then
  ensure_submodule "${ROOT_DIR}/external/googletest" "https://github.com/google/googletest.git"
fi
git -C "${ROOT_DIR}" submodule update --init --recursive
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j
