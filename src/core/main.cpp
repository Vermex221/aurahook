#include <windows.h>
#include "interfaces/interfaces.hpp"
#include "hooks/hooks.hpp"

static DWORD WINAPI init_thread(LPVOID param) {
    HMODULE mod = (HMODULE)param;

    while (!GetModuleHandleA("client.dll")) {
        Sleep(100);
    }

    Sleep(200);

    if (!interfaces::initialize()) {
        return 0;
    }

    if (!hooks::initialize()) {
        return 0;
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hmodule);
        interfaces::dll_module = hmodule;
        HANDLE t = CreateThread(nullptr, 0, init_thread, hmodule, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
