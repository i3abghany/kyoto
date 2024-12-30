#pragma once

#include <optional>
#include <utility>

#include "kyoto/KType.h"

using BinaryInput = std::pair<PrimitiveType::Kind, PrimitiveType::Kind>;

class TypeResolver {
public:
    TypeResolver();

    std::optional<PrimitiveType::Kind> resolve_binary_cmp(PrimitiveType::Kind lhs, PrimitiveType::Kind rhs) const;
    std::optional<PrimitiveType::Kind> resolve_binary_arith(PrimitiveType::Kind lhs, PrimitiveType::Kind rhs) const;
    bool promotable_to(PrimitiveType::Kind from, PrimitiveType::Kind to) const;

private:
};