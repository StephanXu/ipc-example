// Receiver.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Sender.h"

#include <fmt/format.h>
#include "../Func/Func.h"
#include "../Func/timer.h"
#include "../Func/sharememory.h"

#include <condition_variable>
#include <mutex>
#include <string>

// Global Variables:
HINSTANCE hInst; // current instance
std::wstring szCurtPID;
std::wstring szReceiverPID;
std::wstring szNoConnection;
std::wstring szSetConnectionMethod;
std::wstring szConnectionMethod;
std::wstring connectionMethods[3] = {};

Timer refreshTimer;

MemorySharer memorySharer;
SharedMemory* sharedMemory;

HANDLE g_Pipeline{};

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
	SendMessage(GetDlgItem(GetHWND(PROCESS_ROLE_SENDER), IDC_LOG_LIST),
	            LB_ADDSTRING,
	            0,
	            (LPARAM)log.c_str());
	SendDlgItemMessage(GetHWND(PROCESS_ROLE_SENDER),
	                   IDC_LOG_LIST,
	                   LB_SETTOPINDEX,
	                   index++,
	                   0);
};

void SwitchEnable(HWND hWnd, BOOL enable)
{
	EnableWindow(GetDlgItem(hWnd, IDC_MESSAGE_QUEUE), enable);
	EnableWindow(GetDlgItem(hWnd, IDC_MEMORY_MAP), enable);
	EnableWindow(GetDlgItem(hWnd, IDC_PIPELINE), enable);
	EnableWindow(GetDlgItem(hWnd, IDC_MESSAGE_EDIT), enable);
	EnableWindow(GetDlgItem(hWnd, IDC_SEND), enable);
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
	szReceiverPID = LoadStringFromResource(IDS_RECEIVER_PID);
	szNoConnection = LoadStringFromResource(IDS_NO_CONNECTION);
	szSetConnectionMethod = LoadStringFromResource(IDS_SET_CONNECTION_METHOD);
	connectionMethods[CONNECTION_METHOD_MESSAGE_QUEUE] = LoadStringFromResource(IDS_MESSAGE_QUEUE);
	connectionMethods[CONNECTION_METHOD_MEMORY_MAP] = LoadStringFromResource(IDS_MEMORY_MAP);
	connectionMethods[CONNECTION_METHOD_PIPELINE] = LoadStringFromResource(IDS_PIPELINE);
	szConnectionMethod = LoadStringFromResource(IDS_CONNECTION_METHOD);

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_SENDER), 0, WndProc);

	return 0;
}

void CALLBACK WatchPID(UINT uID, UINT uMsg, DWORD dwUsers, DWORD dw1, DWORD dw2)
{
	HWND hWnd{reinterpret_cast<HWND>(dwUsers)};

	DWORD pid = GetProcessIdFromWnd(GetHWND(PROCESS_ROLE_RECEIVER));
	if (!pid)
	{
		SetDlgItemText(hWnd,
		               IDC_RECEIVER_PID,
		               fmt::format(_T("{}[{}]"), szReceiverPID, szNoConnection).c_str());
		SwitchEnable(hWnd, FALSE);
		return;
	}
	SwitchEnable(hWnd, TRUE);
	SetDlgItemText(hWnd,
	               IDC_RECEIVER_PID,
	               fmt::format(_T("{}[{}]"), szReceiverPID, pid).c_str());
	SetDlgItemText(hWnd, IDC_CONNECTION_METHOD,
	               fmt::format(_T("{}{}"), szConnectionMethod, connectionMethods[GetConnectionMethod()]).c_str());
}

std::mutex g_pipelineMutex;
std::condition_variable g_pipelineCv;
WCHAR g_pipelineDataToWrite[0x100]{};
bool g_pipelineReady = false;

DWORD CALLBACK Pipeline(LPVOID param)
{
	if (ConnectNamedPipe(g_Pipeline, nullptr))
	{
		EnableWindow(GetDlgItem(GetHWND(PROCESS_ROLE_SENDER), IDC_PIPELINE), TRUE);
		while (true)
		{
			std::unique_lock<std::mutex> lk(g_pipelineMutex);
			g_pipelineCv.wait(lk, [] { return g_pipelineReady; });
			DWORD written{};
			WriteFile(g_Pipeline,
			          g_pipelineDataToWrite,
			          sizeof(g_pipelineDataToWrite),
			          &written,
			          nullptr);
			NewLog(fmt::format(_T("[{}]<-{}"),
			                   connectionMethods[CONNECTION_METHOD_PIPELINE],
			                   g_pipelineDataToWrite));
			g_pipelineReady = false;
			lk.unlock();
		}
	}
	else
	{
		EnableWindow(GetDlgItem(GetHWND(PROCESS_ROLE_SENDER), IDC_PIPELINE), FALSE);
	}
	return 0;
}

BOOL CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			SetHWND(PROCESS_ROLE_SENDER, hWnd);
			SetDlgItemText(hWnd,
			               IDC_CURT_PID,
			               fmt::format((_T("{}[{}]")), szCurtPID, GetCurrentProcessId()).c_str());

			/* Initialize memory sharing */
			sharedMemory = static_cast<SharedMemory*>(memorySharer.CreateMemory(_T("StephanXu"), sizeof(SharedMemory)));
			ZeroMemory(sharedMemory, sizeof(sharedMemory));

			/* Initialize pipeline */
			g_Pipeline = CreateNamedPipe(
				_T("\\\\.\\pipe\\StephanXuPipeline"),
				PIPE_ACCESS_OUTBOUND,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES,
				0, 0, 0, NULL);

			/* Initialize listening progress */
			refreshTimer.SetTimer(100, WatchPID, reinterpret_cast<DWORD_PTR>(hWnd));
			HANDLE hPipeThread = CreateThread(0, 0, Pipeline, hWnd, 0, 0);
			CloseHandle(hPipeThread);

			/* Create receiver process */
			PROCESS_INFORMATION pi{0};
			STARTUPINFO si{0};
			si.cb = sizeof(si);
			si.dwFlags |= STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
			si.wShowWindow = SW_SHOW;
			CreateProcess(_T("Receiver.exe"),
			              nullptr,
			              nullptr,
			              nullptr,
			              FALSE,
			              NULL,
			              nullptr,
			              nullptr,
			              &si,
			              &pi);
			break;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_SEND:
			switch (GetConnectionMethod())
			{
			case CONNECTION_METHOD_MESSAGE_QUEUE:
				{
					TCHAR buffer[0x100]{};
					GetDlgItemText(hWnd, IDC_MESSAGE_EDIT, buffer, 0x100);
					COPYDATASTRUCT cpy;
					cpy.lpData = buffer;
					cpy.cbData = wcslen(buffer) * sizeof(TCHAR);
					SendMessage(GetHWND(PROCESS_ROLE_RECEIVER),
					            WM_COPYDATA,
					            reinterpret_cast<WPARAM>(GetFocus()),
					            reinterpret_cast<LPARAM>(&cpy));
					NewLog(fmt::format(_T("[{}]<-{}"),
					                   connectionMethods[CONNECTION_METHOD_MESSAGE_QUEUE],
					                   static_cast<TCHAR*>(cpy.lpData)));
					break;
				}
			case CONNECTION_METHOD_MEMORY_MAP:
				{
					WCHAR buffer[0x100]{};
					GetDlgItemTextW(hWnd, IDC_MESSAGE_EDIT, buffer, 0x100);
					memcpy(sharedMemory->m_Buffer, buffer, sizeof(buffer));
					sharedMemory->m_RefreshFlag = true;
					NewLog(fmt::format(_T("[{}]<-{}"),
					                   connectionMethods[CONNECTION_METHOD_MEMORY_MAP],
					                   sharedMemory->m_Buffer));
					break;
				}
			case CONNECTION_METHOD_PIPELINE:
				{
					GetDlgItemTextW(hWnd, IDC_MESSAGE_EDIT, g_pipelineDataToWrite, 0x100);
					g_pipelineReady = true;
					g_pipelineCv.notify_one();
				}
			}
			break;
		case IDC_MESSAGE_QUEUE:
			NewLog(fmt::format(_T("{}{}"), szSetConnectionMethod, connectionMethods[CONNECTION_METHOD_MESSAGE_QUEUE]));
			SetConnectionMethod(CONNECTION_METHOD_MESSAGE_QUEUE);
			break;
		case IDC_MEMORY_MAP:
			NewLog(fmt::format(_T("{}{}"), szSetConnectionMethod, connectionMethods[CONNECTION_METHOD_MEMORY_MAP]));
			SetConnectionMethod(CONNECTION_METHOD_MEMORY_MAP);
			break;
		case IDC_PIPELINE:
			NewLog(fmt::format(_T("{}{}"), szSetConnectionMethod, connectionMethods[CONNECTION_METHOD_PIPELINE]));
			SetConnectionMethod(CONNECTION_METHOD_PIPELINE);
		default:
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
	return 0;
}
