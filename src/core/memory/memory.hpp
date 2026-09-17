#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace memory {

    uintptr_t get_module(const char* name);
    uintptr_t pattern_scan(const char* module_name, const char* pattern);
    uintptr_t resolve_rip(uintptr_t instruction_addr, size_t offset_to_displacement, size_t instruction_size);

    inline bool safe_read(const void* src, void* dest, size_t size) {
        if (!src || !dest || IsBadReadPtr(src, size)) return false;
        memcpy(dest, src, size);
        return true;
    }

    inline bool safe_write(void* dest, const void* src, size_t size) {
        if (!src || !dest || IsBadWritePtr(dest, size)) return false;
        memcpy(dest, src, size);
        return true;
    }

    template <typename T>
    T read(uintptr_t address) {
        if (address < 0x10000 || address >= 0x7FFFFFFFFFFF) return T();
        T val{};
        if (!safe_read(reinterpret_cast<const void*>(address), &val, sizeof(T))) {
            return T();
        }
        return val;
    }

    template <typename T>
    bool write(uintptr_t address, const T& value) {
        if (address < 0x10000 || address >= 0x7FFFFFFFFFFF) return false;
        return safe_write(reinterpret_cast<void*>(address), &value, sizeof(T));
    }

    inline std::string read_string(uintptr_t address, size_t max_len = 64) {
        if (address < 0x10000 || address >= 0x7FFFFFFFFFFF) return "";
        char buf[128] = { 0 };
        size_t len = (max_len < 128) ? max_len : 127;
        for (size_t i = 0; i < len; i++) {
            char c = 0;
            if (!safe_read(reinterpret_cast<const void*>(address + i), &c, 1) || c == '\0') {
                break;
            }
            buf[i] = c;
        }
        return std::string(buf);
    }

    template <typename T>
    T call_vfunc(void* instance, size_t index) {
        if (!instance) return T();
        auto vtable = *reinterpret_cast<void***>(instance);
        return reinterpret_cast<T(__thiscall*)(void*)>(vtable[index])(instance);
    }

    template <typename T, typename... Args>
    T call_vfunc(void* instance, size_t index, Args... args) {
        if (!instance) return T();
        auto vtable = *reinterpret_cast<void***>(instance);
        return reinterpret_cast<T(__thiscall*)(void*, Args...)>(vtable[index])(instance, args...);
    }

}
