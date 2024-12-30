#include "kyoto/AST/NumberNode.h"

#include <string_view>

#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"

#include "kyoto/ModuleCompiler.h"

NumberNode::NumberNode(int64_t value, size_t width, bool sign, ModuleCompiler& compiler)
    : value(value)
    , width(width)
    , sign(sign)
    , compiler(compiler)
{
}

std::string NumberNode::to_string() const
{
    return fmt::format("{}Node({})", get_type(), value);
}

llvm::Value* NumberNode::gen()
{
    return llvm::ConstantInt::get(compiler.get_context(), llvm::APInt(width * 8, value, sign));
}

std::string_view NumberNode::get_type() const
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
        assert(false && "Unknown width");
    }
}