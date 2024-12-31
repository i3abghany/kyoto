#!/bin/bash

GIT_REPO_ROOT=$(git rev-parse --show-toplevel)
cd $GIT_REPO_ROOT

FILES=$(find * -not -path "build/*" | grep -E "^.*(\.h|\.cpp)$")
printf "Formatting files:\n$FILES\n"
echo "$FILES" | xargs clang-format -i