#pragma once

#include <stdint.h>
#include <string>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"

class ModuleCompiler;

class NumberNode : public ExpressionNode {
    int64_t value;
    KType* type;
    ModuleCompiler& compiler;

public:
    NumberNode(int64_t value, KType* ktype, ModuleCompiler& compiler);
    ~NumberNode();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Type* get_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] llvm::Value* trivial_gen();
    [[nodiscard]] bool is_trivially_evaluable() const;

    void cast_to(PrimitiveType::Kind target_type);
};
