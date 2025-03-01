#include "kyoto/TypeResolver.h"

#include <utility>

TypeResolver::TypeResolver() { }

std::optional<PrimitiveType::Kind> TypeResolver::resolve_binary_arith(const PrimitiveType::Kind lhs,
                                                                      const PrimitiveType::Kind rhs) const
{
    if (lhs == rhs && lhs != PrimitiveType::Kind::Boolean) return lhs;
    return std::nullopt;
}

std::optional<PrimitiveType::Kind> TypeResolver::resolve_binary_cmp(const PrimitiveType::Kind lhs,
                                                                    const PrimitiveType::Kind rhs) const
{
    if (lhs == rhs) return PrimitiveType::Kind::Boolean;
    return std::nullopt;
}

std::optional<PrimitiveType::Kind> TypeResolver::resolve_binary_logical(const PrimitiveType::Kind lhs,
                                                                        const PrimitiveType::Kind rhs) const
{
    if (lhs == PrimitiveType::Kind::Boolean && rhs == PrimitiveType::Kind::Boolean) {
        return PrimitiveType::Kind::Boolean;
    }
    return std::nullopt;
}

bool TypeResolver::promotable_to(const PrimitiveType::Kind from, const PrimitiveType::Kind to) const
{
    auto pfrom = PrimitiveType(from);
    auto pto = PrimitiveType(to);
    return (pfrom.is_integer() && pto.is_integer() && pfrom.width() == pto.width())
        || (pfrom.is_boolean() && pto.is_boolean()) || (pfrom.is_floating_point() && pto.is_floating_point());
}

bool TypeResolver::fits_in(const int64_t val, const PrimitiveType::Kind kind) const
{
    auto pkind = PrimitiveType(kind);
    switch (pkind.get_kind()) {
    case PrimitiveType::Kind::I8:
    case PrimitiveType::Kind::Char:
        return std::in_range<int8_t>(val);
    case PrimitiveType::Kind::I16:
        return std::in_range<int16_t>(val);
    case PrimitiveType::Kind::I32:
        return std::in_range<int32_t>(val);
    case PrimitiveType::Kind::I64:
        return std::in_range<int64_t>(val);
    case PrimitiveType::Kind::Boolean:
        return val == 0 || val == 1;
    default:
        return false;
    }
}
