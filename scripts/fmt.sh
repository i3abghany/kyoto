#!/bin/bash

GIT_REPO_ROOT=$(git rev-parse --show-toplevel)
cd $GIT_REPO_ROOT

FILES=$(find * -not -path "build/*" -not -path "cmake-build-debug/*" | grep -E "^.*(\.h|\.cpp)$")
printf "Formatting C++ files:\n$FILES\n"
echo "$FILES" | xargs clang-format -i

RUST_FILES=$(find fuzzer -name "*.rs")
printf "\nFormatting Rust files:\n$RUST_FILES\n"
if ! command -v rustfmt &> /dev/null; then
    echo "rustfmt could not be found."
    exit 1
fi
echo "$RUST_FILES" | xargs rustfmt
    