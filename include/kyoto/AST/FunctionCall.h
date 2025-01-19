#pragma once

#include <string>
#include <vector>

#include "kyoto/AST/ExpressionNode.h"

class ModuleCompiler;

class FunctionCall : ExpressionNode {
public:
    FunctionCall(std::string name, std::vector<ExpressionNode*> args, ModuleCompiler& compiler);
    ~FunctionCall();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type(llvm::LLVMContext& context) const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] const std::string& get_name() const { return name; }
    [[nodiscard]] const std::vector<ExpressionNode*>& get_args() const { return args; }

    [[nodiscard]] bool is_constructor_call() const { return is_constructor; }
    void set_as_constructor_call() { is_constructor = true; }

private:
    std::string name;
    bool is_constructor;
    std::vector<ExpressionNode*> args;
    ModuleCompiler& compiler;
};