#include "kyoto/TypeResolver.h"

#include <cassert>
#include <iostream>
#include <vector>

TypeResolver::TypeResolver()
{
    using Kind = PrimitiveType::Kind;

    std::vector<Kind> types = { Kind::I8, Kind::I16, Kind::I32, Kind::I64, Kind::U8, Kind::U16, Kind::U32, Kind::U64 };

    // FIXME: Fix this abomination
    auto& r = binary_arith_result;
    r[{ Kind::I8, Kind::I8 }] = Kind::I32;
    r[{ Kind::I8, Kind::I16 }] = Kind::I32;
    r[{ Kind::I8, Kind::I32 }] = Kind::I32;
    r[{ Kind::I8, Kind::I64 }] = Kind::I64;
    r[{ Kind::I8, Kind::U8 }] = Kind::U32;
    r[{ Kind::I8, Kind::U16 }] = Kind::U32;
    r[{ Kind::I8, Kind::U32 }] = Kind::U32;
    r[{ Kind::I8, Kind::U64 }] = Kind::U64;
    r[{ Kind::I16, Kind::I8 }] = Kind::I32;
    r[{ Kind::I16, Kind::I16 }] = Kind::I32;
    r[{ Kind::I16, Kind::I32 }] = Kind::I32;
    r[{ Kind::I16, Kind::I64 }] = Kind::I64;
    r[{ Kind::I16, Kind::U8 }] = Kind::U32;
    r[{ Kind::I16, Kind::U16 }] = Kind::U32;
    r[{ Kind::I16, Kind::U32 }] = Kind::U32;
    r[{ Kind::I16, Kind::U64 }] = Kind::U64;
    r[{ Kind::I32, Kind::I8 }] = Kind::I32;
    r[{ Kind::I32, Kind::I16 }] = Kind::I32;
    r[{ Kind::I32, Kind::I32 }] = Kind::I32;
    r[{ Kind::I32, Kind::I64 }] = Kind::I64;
    r[{ Kind::I32, Kind::U8 }] = Kind::U32;
    r[{ Kind::I32, Kind::U16 }] = Kind::U32;
    r[{ Kind::I32, Kind::U32 }] = Kind::U32;
    r[{ Kind::I32, Kind::U64 }] = Kind::U64;
    r[{ Kind::I64, Kind::I8 }] = Kind::I64;
    r[{ Kind::I64, Kind::I16 }] = Kind::I64;
    r[{ Kind::I64, Kind::I32 }] = Kind::I64;
    r[{ Kind::I64, Kind::I64 }] = Kind::I64;
    r[{ Kind::I64, Kind::U8 }] = Kind::I64;
    r[{ Kind::I64, Kind::U16 }] = Kind::I64;
    r[{ Kind::I64, Kind::U32 }] = Kind::I64;
    r[{ Kind::I64, Kind::U64 }] = Kind::U64;
    r[{ Kind::U8, Kind::I8 }] = Kind::U32;
    r[{ Kind::U8, Kind::I16 }] = Kind::U32;
    r[{ Kind::U8, Kind::I32 }] = Kind::U32;
    r[{ Kind::U8, Kind::I64 }] = Kind::I64;
    r[{ Kind::U8, Kind::U8 }] = Kind::U32;
    r[{ Kind::U8, Kind::U16 }] = Kind::U32;
    r[{ Kind::U8, Kind::U32 }] = Kind::U32;
    r[{ Kind::U8, Kind::U64 }] = Kind::U64;
    r[{ Kind::U16, Kind::I8 }] = Kind::U32;
    r[{ Kind::U16, Kind::I16 }] = Kind::U32;
    r[{ Kind::U16, Kind::I32 }] = Kind::U32;
    r[{ Kind::U16, Kind::I64 }] = Kind::I64;
    r[{ Kind::U16, Kind::U8 }] = Kind::U32;
    r[{ Kind::U16, Kind::U16 }] = Kind::U32;
    r[{ Kind::U16, Kind::U32 }] = Kind::U32;
    r[{ Kind::U16, Kind::U64 }] = Kind::U64;
    r[{ Kind::U32, Kind::I8 }] = Kind::U32;
    r[{ Kind::U32, Kind::I16 }] = Kind::U32;
    r[{ Kind::U32, Kind::I32 }] = Kind::U32;
    r[{ Kind::U32, Kind::I64 }] = Kind::I64;
    r[{ Kind::U32, Kind::U8 }] = Kind::U32;
    r[{ Kind::U32, Kind::U16 }] = Kind::U32;
    r[{ Kind::U32, Kind::U32 }] = Kind::U32;
    r[{ Kind::U32, Kind::U64 }] = Kind::U64;
    r[{ Kind::U64, Kind::I8 }] = Kind::U64;
    r[{ Kind::U64, Kind::I16 }] = Kind::U64;
    r[{ Kind::U64, Kind::I32 }] = Kind::U64;
    r[{ Kind::U64, Kind::I64 }] = Kind::U64;
    r[{ Kind::U64, Kind::U8 }] = Kind::U64;
    r[{ Kind::U64, Kind::U16 }] = Kind::U64;
    r[{ Kind::U64, Kind::U32 }] = Kind::U64;
    r[{ Kind::U64, Kind::U64 }] = Kind::U64;
}

PrimitiveType::Kind TypeResolver::resolve_binary_arith(PrimitiveType::Kind lhs, PrimitiveType::Kind rhs) const
{
    if (!binary_arith_result.contains({ lhs, rhs })) {
        std::cerr << "Failed to resolve binary arith type for " << PrimitiveType(lhs).to_string() << " and "
                  << PrimitiveType(rhs).to_string() << std::endl;
        assert(false);
    }

    return binary_arith_result.at({ lhs, rhs });
}