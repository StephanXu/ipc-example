// Receiver.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Receiver.h"
#include <fmt/format.h>
#include "../Func/Func.h"

#include <thread>
#include <string>

// Global Variables:
HINSTANCE hInst; // current instance
std::wstring szCurtPID;
std::wstring szSenderPID;
std::wstring szNoConnection;

BOOL CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

std::wstring LoadStringFromResource(UINT uId)
{
	const size_t MAX_LOADSTRING = 100;
	WCHAR str[MAX_LOADSTRING]{};
	LoadStringW(hInst, uId, str, MAX_LOADSTRING);
	return str;
}

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

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_RECEIVER), 0, WndProc);

	return 0;
}

DWORD WINAPI WatchPID(PVOID param)
{
	HWND hWnd{static_cast<HWND>(param)};
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		DWORD pid = GetProcessIdFromWnd(GetHWND(PROCESS_ROLE_SENDER));
		if (!pid)
		{
			SetDlgItemText(hWnd,
			               IDC_RECEIVER_PID,
			               fmt::format(_T("{}[{}]"), szSenderPID, szNoConnection).c_str());
			continue;
		}
		SetDlgItemText(hWnd,
		               IDC_RECEIVER_PID,
		               fmt::format(_T("{}[{}]"), szSenderPID, pid).c_str());
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

			CloseHandle(CreateThread(
				0,
				0,
				WatchPID,
				hWnd,
				0,
				0));
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
