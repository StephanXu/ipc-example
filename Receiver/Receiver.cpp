// Receiver.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Receiver.h"
#include <fmt/format.h>
#include "../Func/Func.h"
#include "../Func/timer.h"
#include "../Func/sharememory.h"

#include <string>


// Global Variables:
HINSTANCE hInst; // current instance
std::wstring szCurtPID;
std::wstring szSenderPID;
std::wstring szNoConnection;
std::wstring szSetConnectionMethod;
std::wstring szConnectionMethod;
std::wstring connectionMethods[3] = {};

Timer refreshTimer;

MemorySharer memorySharer;
SharedMemory* sharedMemory;

BOOL CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

std::wstring LoadStringFromResource(UINT uId)
{
	const size_t MAX_LOADSTRING = 100;
	WCHAR str[MAX_LOADSTRING]{};
	LoadStringW(hInst, uId, str, MAX_LOADSTRING);
	return str;
}

void NewLog(std::wstring log)
{
	static int index = 0;
	SendMessage(GetDlgItem(GetHWND(PROCESS_ROLE_RECEIVER), IDC_LOG_LIST),
	            LB_ADDSTRING,
	            0,
	            (LPARAM)log.c_str());
	SendDlgItemMessage(GetHWND(PROCESS_ROLE_RECEIVER),
	                   IDC_LOG_LIST,
	                   LB_SETTOPINDEX,
	                   index++,
	                   0);
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	hInst = hInstance;

	szCurtPID = LoadStringFromResource(IDS_CURT_PID);
	szSenderPID = LoadStringFromResource(IDS_SENDER_PID);
	szNoConnection = LoadStringFromResource(IDS_NO_CONNECTION);
	szSetConnectionMethod = LoadStringFromResource(IDS_SET_CONNECTION_METHOD);
	connectionMethods[CONNECTION_METHOD_MESSAGE_QUEUE] = LoadStringFromResource(IDS_MESSAGE_QUEUE);
	connectionMethods[CONNECTION_METHOD_MEMORY_MAP] = LoadStringFromResource(IDS_MEMORY_MAP);
	connectionMethods[CONNECTION_METHOD_PIPELINE] = LoadStringFromResource(IDS_PIPELINE);
	szConnectionMethod = LoadStringFromResource(IDS_CONNECTION_METHOD);

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_RECEIVER), 0, WndProc);

	return 0;
}

void CALLBACK WatchPID(UINT uID, UINT uMsg, DWORD dwUsers, DWORD dw1, DWORD dw2)
{
	HWND hWnd{reinterpret_cast<HWND>(dwUsers)};

	DWORD pid = GetProcessIdFromWnd(GetHWND(PROCESS_ROLE_SENDER));
	if (!pid)
	{
		SetDlgItemText(hWnd,
		               IDC_RECEIVER_PID,
		               fmt::format(_T("{}[{}]"), szSenderPID, szNoConnection).c_str());
		return;
	}
	SetDlgItemText(hWnd,
	               IDC_RECEIVER_PID,
	               fmt::format(_T("{}[{}]"), szSenderPID, pid).c_str());
	SetDlgItemText(hWnd, IDC_CONNECTION_METHOD,
	               fmt::format(_T("{}{}"), szConnectionMethod, connectionMethods[GetConnectionMethod()]).c_str());
	if (sharedMemory->m_RefreshFlag)
	{
		NewLog(fmt::format(_T("[{}]->{}"), connectionMethods[CONNECTION_METHOD_MEMORY_MAP], sharedMemory->m_Buffer));
		sharedMemory->m_RefreshFlag = false;
	}
}

DWORD CALLBACK Pipeline(LPVOID param)
{
	HANDLE hPipe = CreateFile(
		_T("\\\\.\\pipe\\StephanXuPipeline"),
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (GetLastError() != 0)
	{
		return -1;
	}
	DWORD dwMode = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(
		hPipe,
		&dwMode,
		NULL,
		NULL);
	WCHAR buffer[0x100]{};
	BOOL fSuccess{};
	while (GetProcessIdFromWnd(GetHWND(PROCESS_ROLE_SENDER)))
	{
		DWORD cbRead;
		fSuccess = ReadFile(
			hPipe,
			buffer,
			0x100 * sizeof(WCHAR),
			&cbRead,
			NULL);
		if (cbRead > 0)
		{
			NewLog(fmt::format(_T("[{}]->{}"),
			                   connectionMethods[CONNECTION_METHOD_PIPELINE],
			                   buffer));
		}
	}
	return 0;
}

BOOL CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			SetHWND(PROCESS_ROLE_RECEIVER, hWnd);
			SetDlgItemText(hWnd,
			               IDC_CURT_PID,
			               fmt::format((_T("{}[{}]")), szCurtPID, GetCurrentProcessId()).c_str());
			sharedMemory = static_cast<SharedMemory*>(memorySharer.GetMemory("StephanXu"));
			refreshTimer.SetTimer(100, WatchPID, reinterpret_cast<DWORD_PTR>(hWnd));
			HANDLE hPipeThread = CreateThread(0, 0, Pipeline, hWnd, 0, 0);
			CloseHandle(hPipeThread);
			break;
		}
	case WM_COPYDATA:
		{
			COPYDATASTRUCT* cpy = reinterpret_cast<COPYDATASTRUCT*>(lParam);
			NewLog(fmt::format(_T("[{}]->{}"),
			                   connectionMethods[CONNECTION_METHOD_MESSAGE_QUEUE],
			                   static_cast<TCHAR*>(cpy->lpData)));
			break;
		}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
