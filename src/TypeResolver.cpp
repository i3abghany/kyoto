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

bool TypeResolver::promotable_to(PrimitiveType::Kind from, PrimitiveType::Kind to) const
{
    auto pfrom = PrimitiveType(from);
    auto pto = PrimitiveType(to);
    return pfrom.is_integer() && pto.is_integer() || pfrom.is_floating_point() && pto.is_floating_point();
}