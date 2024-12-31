#pragma once

#include <optional>
#include <stdint.h>

#include "kyoto/KType.h"

class TypeResolver {
public:
    TypeResolver();

    std::optional<PrimitiveType::Kind> resolve_binary_cmp(PrimitiveType::Kind lhs, PrimitiveType::Kind rhs) const;
    std::optional<PrimitiveType::Kind> resolve_binary_arith(PrimitiveType::Kind lhs, PrimitiveType::Kind rhs) const;
    bool promotable_to(PrimitiveType::Kind from, PrimitiveType::Kind to) const;
    bool fits_in(int64_t val, PrimitiveType::Kind kind) const;

private:
};