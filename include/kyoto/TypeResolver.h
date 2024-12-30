#pragma once

#include <map>
#include <utility>

#include "kyoto/KType.h"

using BinaryInput = std::pair<PrimitiveType::Kind, PrimitiveType::Kind>;

class TypeResolver {
public:
    TypeResolver();

    PrimitiveType::Kind resolve_binary_arith(PrimitiveType::Kind lhs, PrimitiveType::Kind rhs) const;
    BinaryInput promote_to(PrimitiveType::Kind lhs, PrimitiveType::Kind rhs) const;

private:
    std::map<BinaryInput, PrimitiveType::Kind> binary_arith_result;
};