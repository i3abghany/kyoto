#include <fmt/core.h>
#include <iostream>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include "kyoto/ModuleCompiler.h"

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string to_string() const = 0;
    virtual llvm::Value* gen() = 0;
};

class IntNode : public ASTNode {
    int64_t value;
    size_t width;
    ModuleCompiler& compiler;

public:
    IntNode(int64_t value, size_t width, ModuleCompiler& compiler)
        : value(value)
        , width(width)
        , compiler(compiler)
    {
    }

    std::string to_string() const override { return fmt::format("{}Node({})", get_type(), value); }

    llvm::Value* gen() override
    {
        return llvm::ConstantInt::get(compiler.get_context(), llvm::APInt(width * 8, value, true));
    }

private:
    std::string_view get_type() const
    {
        switch (width) {
        case 8:
            return "I8";
        case 16:
            return "I16";
        case 32:
            return "I32";
        case 64:
            return "I64";
        default:
            return "I64";
        }
    }
};

class AddNode : public ASTNode {
    ASTNode* lhs;
    ASTNode* rhs;
    ModuleCompiler& compiler;

public:
    AddNode(ASTNode* lhs, ASTNode* rhs, ModuleCompiler& compiler)
        : lhs(lhs)
        , rhs(rhs)
        , compiler(compiler)
    {
    }

    std::string to_string() const override
    {
        return fmt::format("AddNode({}, {})", lhs->to_string(), rhs->to_string());
    }

    llvm::Value* gen() override
    {
        auto* lhs_val = lhs->gen();
        auto* rhs_val = rhs->gen();
        // TODO: this adds integers only for now
        return compiler.get_builder().CreateAdd(lhs_val, rhs_val, "addval");
    }
};

class ReturnStatementNode : public ASTNode {
    ASTNode* expr;
    ModuleCompiler& compiler;

public:
    ReturnStatementNode(ASTNode* expr, ModuleCompiler& compiler)
        : expr(expr)
        , compiler(compiler)
    {
    }

    std::string to_string() const override { return fmt::format("ReturnNode({})", expr->to_string()); }

    llvm::Value* gen() override
    {
        auto* expr_val = expr->gen();
        return compiler.get_builder().CreateRet(expr_val);
    }
};

struct Parameter {
    std::string name;
    std::string type;
};

class FunctionNode : public ASTNode {
    std::string name;
    std::vector<Parameter> args;
    std::string ret_type;
    std::vector<ASTNode*> body;
    ModuleCompiler& compiler;

public:
    FunctionNode(const std::string& name, std::vector<Parameter> args, std::string ret_type, std::vector<ASTNode*> body,
                 ModuleCompiler& compiler)
        : name(name)
        , args(std::move(args))
        , ret_type(std::move(ret_type))
        , body(std::move(body))
        , compiler(compiler)
    {
    }

    std::string to_string() const override
    {
        std::string args_str;
        for (const auto& arg : args) {
            args_str += arg.name + ": " + arg.type + ", ";
        }
        return fmt::format("FunctionNode({}, [{}], [{}])", name, args_str, body.size());
    }

    llvm::Value* gen() override
    {
        auto* return_type = get_type(ret_type, compiler.get_context());
        auto* func_type = llvm::FunctionType::get(return_type, get_arg_types(), false);
        auto* func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, compiler.get_module());
        auto* entry = llvm::BasicBlock::Create(compiler.get_context(), "func_entry", func);
        compiler.get_builder().SetInsertPoint(entry);

        for (auto* node : body) {
            node->gen();
        }

        return func;
    }

private:
    std::vector<llvm::Type*> get_arg_types() const
    {
        std::vector<llvm::Type*> types;
        for (const auto& arg : args) {
            auto* type = get_type(arg.type, compiler.get_context());
            types.push_back(type);
        }
        return types;
    }

    static llvm::Type* get_type(const std::string& type, llvm::LLVMContext& context)
    {
        if (type == "i8") {
            return llvm::Type::getInt8Ty(context);
        } else if (type == "i16") {
            return llvm::Type::getInt16Ty(context);
        } else if (type == "i32") {
            return llvm::Type::getInt32Ty(context);
        } else if (type == "i64") {
            return llvm::Type::getInt64Ty(context);
        } else {
            assert(false && "Unknown type");
        }
        return nullptr;
    }
};