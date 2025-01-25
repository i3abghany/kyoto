#pragma once

#include <format>
#include <stddef.h>
#include <string>
#include <vector>

#include "kyoto/AST/ASTNode.h"
#include "kyoto/AST/Expressions/ExpressionNode.h"

class ModuleCompiler;

class FunctionCall : public ExpressionNode {
public:
    FunctionCall(std::string name, std::vector<ExpressionNode*> args, ModuleCompiler& compiler);
    ~FunctionCall();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] llvm::Value* gen() override;
    [[nodiscard]] llvm::Value* gen_ptr() const override;
    [[nodiscard]] llvm::Type* gen_type() const override;
    [[nodiscard]] KType* get_ktype() const override;
    [[nodiscard]] llvm::Value* trivial_gen() override;
    [[nodiscard]] bool is_trivially_evaluable() const override;

    [[nodiscard]] const std::string& get_name() const { return name; }
    [[nodiscard]] const std::vector<ExpressionNode*>& get_args() const { return args; }

    void insert_arg(ExpressionNode* node, size_t index);

    [[nodiscard]] std::vector<ASTNode*> get_children() const override { return { args.begin(), args.end() }; }

    [[nodiscard]] bool is_constructor_call() const { return is_constructor; }
    void set_as_constructor_call()
    {
        is_constructor = true;
        name += "_constructor";
    }

private:
    std::string name;
    bool is_constructor { false };
    std::vector<ExpressionNode*> args;
    ModuleCompiler& compiler;
};