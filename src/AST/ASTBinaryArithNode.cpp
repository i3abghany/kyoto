#include <assert.h>
#include <fmt/core.h>
#include <optional>
#include <stdexcept>
#include <string>

#include "kyoto/AST/ASTBinaryNode.h"
#include "kyoto/AST/ASTNode.h"
#include "kyoto/KType.h"
#include "kyoto/ModuleCompiler.h"
#include "kyoto/TypeResolver.h"
#include "llvm/IR/IRBuilder.h"

#define ARITH_BINARY_NODE_IMPL_BASE(name, op, llvm_op)                                                          \
    name::name(ExpressionNode* lhs, ExpressionNode* rhs, ModuleCompiler& compiler)                              \
        : lhs(lhs)                                                                                              \
        , rhs(rhs)                                                                                              \
        , compiler(compiler)                                                                                    \
    {                                                                                                           \
    }                                                                                                           \
    name::~name()                                                                                               \
    {                                                                                                           \
        delete lhs;                                                                                             \
        delete rhs;                                                                                             \
    }                                                                                                           \
    std::string name::to_string() const                                                                         \
    {                                                                                                           \
        return fmt::format("{}({}, {})", #name, lhs->to_string(), rhs->to_string());                            \
    }                                                                                                           \
    llvm::Value* name::gen()                                                                                    \
    {                                                                                                           \
        auto* lhs_val = lhs->gen();                                                                             \
        auto* rhs_val = rhs->gen();                                                                             \
        auto lhs_ktype = PrimitiveType::from_llvm_type(lhs->get_type(compiler.get_context()));                  \
        auto rhs_ktype = PrimitiveType::from_llvm_type(rhs->get_type(compiler.get_context()));                  \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype.get_kind(), rhs_ktype.get_kind()); \
        if (!t.has_value())                                                                                     \
            throw std::runtime_error(fmt::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op, \
                                                 lhs_ktype.to_string(), rhs_ktype.to_string()));                \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");                                     \
    }                                                                                                           \
    llvm::Type* name::get_type(llvm::LLVMContext& context) const                                                \
    {                                                                                                           \
        auto lhs_ktype = PrimitiveType::from_llvm_type(lhs->get_type(context));                                 \
        auto rhs_ktype = PrimitiveType::from_llvm_type(rhs->get_type(context));                                 \
        auto t = compiler.get_type_resolver().resolve_binary_arith(lhs_ktype.get_kind(), rhs_ktype.get_kind()); \
        if (!t.has_value())                                                                                     \
            throw std::runtime_error(fmt::format("Operator `{}` cannot be applied to types `{}` and `{}`", #op, \
                                                 lhs_ktype.to_string(), rhs_ktype.to_string()));                \
        auto ktype = new PrimitiveType(t.value());                                                              \
        auto res = ASTNode::get_llvm_type(ktype, context);                                                      \
        delete ktype;                                                                                           \
        return res;                                                                                             \
    }

#define ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL(name, op, llvm_op)            \
    ARITH_BINARY_NODE_IMPL_BASE(name, op, llvm_op)                             \
    llvm::Value* name::trivial_gen()                                           \
    {                                                                          \
        assert(is_trivially_evaluable());                                      \
        auto* lhs_val = lhs->trivial_gen();                                    \
        auto* rhs_val = rhs->trivial_gen();                                    \
        return compiler.get_builder().llvm_op(lhs_val, rhs_val, #op "val");    \
    }                                                                          \
    bool name::is_trivially_evaluable() const                                  \
    {                                                                          \
        return lhs->is_trivially_evaluable() && rhs->is_trivially_evaluable(); \
    }

#define ARITH_BINARY_NODE_IMPL_NO_TRIVIAL_EVAL(name, op, llvm_op) \
    ARITH_BINARY_NODE_IMPL_BASE(name, op, llvm_op)                \
    llvm::Value* name::trivial_gen()                              \
    {                                                             \
        return nullptr;                                           \
    }                                                             \
    bool name::is_trivially_evaluable() const                     \
    {                                                             \
        return false;                                             \
    }

ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL(MulNode, Mul, CreateMul);
ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL(AddNode, Add, CreateAdd);
ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL(SubNode, Sub, CreateSub);
ARITH_BINARY_NODE_IMPL_NO_TRIVIAL_EVAL(DivNode, Div, CreateSDiv);
ARITH_BINARY_NODE_IMPL_NO_TRIVIAL_EVAL(ModNode, Mod, CreateSRem);

#undef ARITH_BINARY_NODE_IMPL_WITH_TRIVIAL_EVAL
#undef ARITH_BINARY_NODE_IMPL_NO_TRIVIAL_EVAL
#undef ARITH_BINARY_NODE_IMPL_BASE
