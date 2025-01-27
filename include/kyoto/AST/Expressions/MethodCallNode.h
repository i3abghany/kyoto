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
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] const std::string& get_instance_name() const { return instance_name; }
    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return FunctionCall::get_children(); }

    void initalize_name_prefix() const;

private:
    std::string instance_name;
};