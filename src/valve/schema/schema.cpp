#include "schema.hpp"
#include "../../core/interfaces/interfaces.hpp"
#include "../../core/memory/memory.hpp"

namespace schema {

    uint32_t lookup(const char* class_name, uint32_t field_hash) {
        if (!interfaces::schema_system) return 0;

        uintptr_t type_scope = memory::call_vfunc<uintptr_t>(interfaces::schema_system, 13, "client.dll", nullptr);
        if (!type_scope) return 0;

        uintptr_t class_info = 0;
        memory::call_vfunc<void>(reinterpret_cast<void*>(type_scope), 2, &class_info, class_name);
        if (!class_info) return 0;

        uintptr_t fields_ptr = memory::read<uintptr_t>(class_info + 0x30);
        uint16_t field_count = memory::read<uint16_t>(class_info + 0x24);

        if (!fields_ptr || !field_count) return 0;

        for (uint16_t i = 0; i < field_count; ++i) {
            uintptr_t field_addr = fields_ptr + (static_cast<size_t>(i) * 0x20);
            uintptr_t name_ptr = memory::read<uintptr_t>(field_addr);

            if (!name_ptr) continue;

            if (fnv1a::runtime_hash(reinterpret_cast<const char*>(name_ptr)) == field_hash) {
                return memory::read<uint32_t>(field_addr + 0x10);
            }
        }

        return 0;
    }

}

