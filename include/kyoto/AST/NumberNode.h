#pragma once

#include <string>

#include "ASTNode.h"

class ModuleCompiler;

namespace llvm {
class Value;
}

class NumberNode : public ASTNode {
    int64_t value;
    size_t width;
    bool sign;
    ModuleCompiler& compiler;

public:
    NumberNode(int64_t value, size_t width, bool sign, ModuleCompiler& compiler);

    [[nodiscard]] std::string to_string() const override;
    llvm::Value* gen() override;

private:
    [[nodiscard]] std::string_view get_type() const;
};
