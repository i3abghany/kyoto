#include "kyoto/TypeResolver.h"

TypeResolver::TypeResolver() { }

std::optional<PrimitiveType::Kind> TypeResolver::resolve_binary_arith(PrimitiveType::Kind lhs,
                                                                      PrimitiveType::Kind rhs) const
{
    // FIXME: figure out whether we want to do an elaborate promotion scheme
    // like the C standard mandates
    if (lhs == rhs)
        return lhs;
    return std::nullopt;
}

std::optional<PrimitiveType::Kind> TypeResolver::resolve_binary_cmp(PrimitiveType::Kind lhs,
                                                                    PrimitiveType::Kind rhs) const
{
    if (lhs == rhs)
        return PrimitiveType::Kind::Boolean;
    return std::nullopt;
}

bool TypeResolver::promotable_to(PrimitiveType::Kind from, PrimitiveType::Kind to) const
{
    auto pfrom = PrimitiveType(from);
    auto pto = PrimitiveType(to);
    return pfrom.is_integer() && pto.is_integer() || pfrom.is_floating_point() && pto.is_floating_point();
}

bool TypeResolver::fits_in(int64_t val, PrimitiveType::Kind kind) const
{
    auto pkind = PrimitiveType(kind);
    switch (pkind.get_kind()) {
    case PrimitiveType::Kind::I8:
        return val >= INT8_MIN && val <= INT8_MAX;
    case PrimitiveType::Kind::I16:
        return val >= INT16_MIN && val <= INT16_MAX;
    case PrimitiveType::Kind::I32:
        return val >= INT32_MIN && val <= INT32_MAX;
    case PrimitiveType::Kind::I64:
        return val >= INT64_MIN && val <= INT64_MAX;
    default:
        break;
    }
    return false;
}