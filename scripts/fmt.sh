#!/bin/bash

FILES=$(find * -not -path "build/*" | grep -E "^.*(\.h|\.cpp)$")
printf "Formatting files:\n$FILES\n"
echo "$FILES" | xargs clang-format -i