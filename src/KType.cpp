#include "kyoto/KType.h"

#include <assert.h>

#include "llvm/IR/Type.h"

PrimitiveType::PrimitiveType(Kind kind)
    : kind(kind)
{
}

std::string PrimitiveType::to_string() const
{
    switch (kind) {
    case Kind::Boolean:
        return "Boolean";
    case Kind::Char:
        return "Char";
    case Kind::I8:
        return "I8";
    case Kind::I16:
        return "I16";
    case Kind::I32:
        return "I32";
    case Kind::I64:
        return "I64";
    case Kind::F32:
        return "F32";
    case Kind::F64:
        return "F64";
    case Kind::String:
        return "String";
    case Kind::Void:
        return "Void";
    case Kind::Unknown:
        return "Unknown";
    }
    return "Unknown";
}

bool PrimitiveType::is_integer() const
{
    return kind == Kind::I8 || kind == Kind::I16 || kind == Kind::I32 || kind == Kind::I64;
}

bool PrimitiveType::is_floating_point() const
{
    return kind == Kind::F32 || kind == Kind::F64;
}

bool PrimitiveType::is_boolean() const
{
    return kind == Kind::Boolean;
}

bool PrimitiveType::is_numeric() const
{
    return is_integer() || is_floating_point();
}

bool PrimitiveType::is_char() const
{
    return kind == Kind::Char;
}

size_t PrimitiveType::width() const
{
    switch (kind) {
    case Kind::Boolean:
    case Kind::Char:
    case Kind::I8:
        return 1;

    case Kind::I16:
        return 2;

    case Kind::I32:
    case Kind::F32:
        return 4;

    case Kind::I64:
    case Kind::F64:
        return 8;

    default:
        break;
    }

    assert(false && "Unknown type");
}

PrimitiveType::Kind PrimitiveType::get_kind() const
{
    return kind;
}

PrimitiveType PrimitiveType::from_llvm_type(const llvm::Type* type)
{
    if (type->isIntegerTy(1)) return PrimitiveType(Kind::Boolean);
    if (type->isIntegerTy(8)) return PrimitiveType(Kind::I8);
    if (type->isIntegerTy(16)) return PrimitiveType(Kind::I16);
    if (type->isIntegerTy(32)) return PrimitiveType(Kind::I32);
    if (type->isIntegerTy(64)) return PrimitiveType(Kind::I64);

    if (type->isFloatTy()) return PrimitiveType(Kind::F32);
    if (type->isDoubleTy()) return PrimitiveType(Kind::F64);

    return PrimitiveType(Kind::Unknown);
}