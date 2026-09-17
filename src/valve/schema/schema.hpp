#pragma once

#include <cstdint>
#include <cstddef>
#include "../sdk.hpp"

namespace schema {
    uint32_t lookup(const char* class_name, uint32_t field_hash);
}

#define SCHEMA_FIELD(type, name, mod, netvar_class, field) \
    type& name() { \
        static const auto _off = schema::lookup(netvar_class, fnv1a::hash(field, sizeof(field) - 1)); \
        return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + _off); \
    }
