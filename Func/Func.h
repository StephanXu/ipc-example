#pragma once

#include <Windows.h>

#ifndef FUNC_H
#define FUNC_H

#ifdef FUNC_NO_EXPORT
#define FUNC_API
#else
#ifdef FUNC_EXPORTS
#define FUNC_API __declspec(dllexport)
#else
#define FUNC_API __declspec(dllimport)
#endif //FUNC_EXPORT
#endif //FUNC_NO_EXPORT

#ifndef _WIN32
#ifndef __stdcall
#define __stdcall __attribute__((stdcall))
#endif //!__stdcall
#endif //_WIN32


enum ProcessRole
{
	PROCESS_ROLE_SENDER = 0,
	PROCESS_ROLE_RECEIVER
};

enum ConnectionMethod
{
	CONNECTION_METHOD_MESSAGE_QUEUE = 0,
	CONNECTION_METHOD_MEMORY_MAP,
	CONNECTION_METHOD_PIPELINE
};

struct SharedMemory
{
	bool m_RefreshFlag;
	WCHAR m_Buffer[0x100];
};

extern "C" {
FUNC_API VOID WINAPI SetHWND(const ProcessRole role, const HWND hWnd);
FUNC_API HWND WINAPI GetHWND(const ProcessRole role);
FUNC_API DWORD WINAPI GetProcessIdFromWnd(HWND hWnd);
FUNC_API VOID WINAPI SetConnectionMethod(const ConnectionMethod connectionMethod);
FUNC_API ConnectionMethod WINAPI GetConnectionMethod();
}

#endif // FUNC_H
