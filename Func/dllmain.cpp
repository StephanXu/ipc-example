// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Func.h"
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#pragma data_seg("meta")
HWND g_wnd[2] = { nullptr };
int g_connMethod{};
#pragma data_seg()
#pragma comment(linker,"/SECTION:meta,RWS")

FUNC_API VOID WINAPI SetHWND(const ProcessRole role, const HWND hWnd)
{
    g_wnd[role] = hWnd;
}

FUNC_API HWND WINAPI GetHWND(const ProcessRole role)
{
    return g_wnd[role];
}

FUNC_API DWORD WINAPI GetProcessIdFromWnd(HWND hWnd)
{
    DWORD pid{};
    GetWindowThreadProcessId(hWnd, &pid);
    return pid;
}

FUNC_API VOID WINAPI SetConnectionMethod(const ConnectionMethod connectionMethod)
{
    g_connMethod = connectionMethod;
}

FUNC_API ConnectionMethod WINAPI GetConnectionMethod()
{
    return (ConnectionMethod)g_connMethod;
}