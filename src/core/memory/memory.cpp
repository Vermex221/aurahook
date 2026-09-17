#include "memory.hpp"
#include <psapi.h>

namespace memory {

    uintptr_t get_module(const char* name) {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA(name));
    }

    uintptr_t pattern_scan(const char* module_name, const char* pattern) {
        uintptr_t module_base = get_module(module_name);
        if (!module_base) return 0;

        MODULEINFO module_info{};
        GetModuleInformation(GetCurrentProcess(), reinterpret_cast<HMODULE>(module_base), &module_info, sizeof(module_info));

        std::vector<int> pattern_bytes;
        const char* start = pattern;
        const char* end = pattern + strlen(pattern);

        for (const char* current = start; current < end; ++current) {
            if (*current == '?') {
                if (*(current + 1) == '?') ++current;
                pattern_bytes.push_back(-1);
            } else if (*current != ' ') {
                pattern_bytes.push_back(static_cast<int>(strtoul(current, const_cast<char**>(&current), 16)));
            }
        }

        uint8_t* scan_bytes = reinterpret_cast<uint8_t*>(module_base);
        size_t pattern_size = pattern_bytes.size();
        size_t image_size = module_info.SizeOfImage;

        if (image_size <= pattern_size) return 0;

        for (size_t i = 0; i < image_size - pattern_size; ++i) {
            bool found = true;
            for (size_t j = 0; j < pattern_size; ++j) {
                if (pattern_bytes[j] != -1 && scan_bytes[i + j] != pattern_bytes[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return reinterpret_cast<uintptr_t>(&scan_bytes[i]);
            }
        }

        return 0;
    }

    uintptr_t resolve_rip(uintptr_t instruction_addr, size_t offset_to_displacement, size_t instruction_size) {
        if (!instruction_addr) return 0;
        int32_t disp = read<int32_t>(instruction_addr + offset_to_displacement);
        return instruction_addr + instruction_size + disp;
    }

}
