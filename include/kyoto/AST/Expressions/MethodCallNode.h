#pragma once

#include <kyoto/AST/Expressions/FunctionCallNode.h>

class ModuleCompiler;

class MethodCall : public FunctionCall {
public:
    MethodCall(std::string instance_name, std::string name, std::vector<ExpressionNode*> args,
               ModuleCompiler& compiler);
    ~MethodCall();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;

    [[nodiscard]] const std::string& get_instance_name() const { return instance_name; }

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return FunctionCall::get_children(); }

private:
    std::string instance_name;
};