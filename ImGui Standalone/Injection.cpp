#include "Injection.h"
#include "Tools.h"

#include <string>
#include <filesystem>
#include <psapi.h>
#include <thread>
#include <tlhelp32.h>
#include <wtsapi32.h>
#pragma comment(lib, "Wtsapi32.lib")

struct WindowTitleData {
	DWORD pid;
	std::wstring wNewTitle;
	bool foundWindow;
};


std::vector<std::string> RunCommandWithOutput(const std::string& command) {
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;


	HANDLE hReadPipe, hWritePipe;
	if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
		MessageBoxA(0, "Error -> Couldn't pipe Handle64?? returning {}", "D2RMulti", 0);
		return {};
	}

	SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = hWritePipe;
	si.hStdError = hWritePipe;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	char cmdBuffer[512];
	strncpy_s(cmdBuffer, command.c_str(), sizeof(cmdBuffer) - 1);
	cmdBuffer[sizeof(cmdBuffer) - 1] = '\0';
	if (!CreateProcessA(NULL, cmdBuffer, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		MessageBoxA(0, "Error -> Couldn't CreateProcess Handle64... returning {}", "D2RMulti", 0);
		CloseHandle(hReadPipe);
		CloseHandle(hWritePipe);
		return {};
	}

	CloseHandle(hWritePipe);

	std::string					outputChunk;
	DWORD						bytesRead;
	std::vector<std::string>	outputLines;

	char buffer[4096];
	while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
		buffer[bytesRead] = 0;
		outputChunk += buffer;
	}

	CloseHandle(hReadPipe);
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	std::istringstream stream(outputChunk);
	std::string line;
	while (std::getline(stream, line)) {
		outputLines.push_back(line);
	}
	return outputLines;
}

DWORD RunCommand(const std::string& command) {
	STARTUPINFOA si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	DWORD creationFlags = CREATE_BREAKAWAY_FROM_JOB | CREATE_NO_WINDOW;

	if (!CreateProcessA(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, creationFlags, NULL, NULL, &si, &pi)) {
		MessageBoxA(0, "Error: CreateProcess failed ...", "D2RMulti", MB_ICONERROR | MB_OK);
		return 0;
	}

	DWORD pid = pi.dwProcessId;

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return pid;
}

void EjectDLL(int pid, const std::string& dllName) {
	std::wstring wpath(dllName.begin(), dllName.end());
	std::wstring wideDllName = std::filesystem::path(wpath).filename();

	HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
	if (!hKernel32) {
		MessageBoxA(NULL, "Failed to get handle to kernel32.dll", "Error", MB_ICONERROR | MB_OK);
		return;
	}

	HANDLE hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, pid);
	if (!hProc) {
		MessageBoxA(NULL, "Failed to open process", "Error", MB_ICONERROR | MB_OK);
		return;
	}

	LPTHREAD_START_ROUTINE lpFreeLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "FreeLibrary");
	if (!lpFreeLibrary) {
		MessageBoxA(NULL, "Failed to get FreeLibrary address", "Error", MB_ICONERROR | MB_OK);
		CloseHandle(hProc);
		return;
	}

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		MessageBoxA(NULL, "Failed to create tool help snapshot", "Error", MB_ICONERROR | MB_OK);
		CloseHandle(hProc);
		return;
	}

	MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
	LPVOID moduleBaseAddr = nullptr;
	bool found = false;
	if (Module32First(hSnapshot, &me32)) {
		do {
			if (_wcsicmp(me32.szModule, wideDllName.c_str()) == 0) {
				moduleBaseAddr = me32.modBaseAddr;
				found = true;
				break;
			}
		} while (Module32Next(hSnapshot, &me32));
	}
	CloseHandle(hSnapshot);

	if (!found) {
		//MessageBoxA(NULL, "DLL not found in process", "Error", MB_ICONERROR | MB_OK);
		CloseHandle(hProc);
		return;
	}

	int attempt = 1;
	while (found) {
		HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, lpFreeLibrary, moduleBaseAddr, 0, NULL);
		if (!hThread) {
			MessageBoxA(NULL, "Failed to create remote thread", "Error", MB_ICONERROR | MB_OK);
			CloseHandle(hProc);
			return;
		}

		WaitForSingleObject(hThread, INFINITE);
		DWORD exitCode;
		GetExitCodeThread(hThread, &exitCode);
		CloseHandle(hThread);
		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			std::wcout << L"[+] Failed to recheck module snapshot." << std::endl;
			break;
		}

		found = false;
		if (Module32First(hSnapshot, &me32)) {
			do {
				if (_wcsicmp(me32.szModule, wideDllName.c_str()) == 0) {
					found = true;
					break;
				}
			} while (Module32Next(hSnapshot, &me32));
		}
		CloseHandle(hSnapshot);

		if (found) {
			std::wcout << L"[+] DLL still loaded, retrying..." << std::endl;
		}
		else {
			std::wcout << L"[+] DLL ejected successfully after " << attempt << L" attempts." << std::endl;
		}
		attempt++;
	}
	CloseHandle(hProc);
}
void InjectDLL(const int& pid, const std::string& path) {
	if (!std::filesystem::exists(path)) {
		MessageBoxA(0, "Error: Couldn't find DLL...", "D2RMulti", 0);
		return;
	}

	std::wstring wpath(path.begin(), path.end());
	long dll_size = static_cast<long>((wpath.length() + 1) * sizeof(wchar_t));

	std::cout << "[+] Opening process " << pid << " for injection..." << std::endl;
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProc == NULL) {
		DWORD error = GetLastError();
		std::cout << "[!] OpenProcess failed with error: " << error << std::endl;
		MessageBoxA(0, "Error: Failed to open target process...", "D2RMulti", 0);
		return;
	}

	std::cout << "[+] Allocating memory in process..." << std::endl;
	LPVOID lpAlloc = VirtualAllocEx(hProc, NULL, dll_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (lpAlloc == NULL) {
		DWORD error = GetLastError();
		std::cout << "[!] VirtualAllocEx failed with error: " << error << std::endl;
		MessageBoxA(0, "Error: VirtualAllocEx failed...", "D2RMulti", 0);
		CloseHandle(hProc);
		return;
	}

	std::cout << "[+] Writing DLL path to process memory..." << std::endl;
	if (WriteProcessMemory(hProc, lpAlloc, wpath.c_str(), dll_size, NULL) == 0) {
		DWORD error = GetLastError();
		std::cout << "[!] WriteProcessMemory failed with error: " << error << std::endl;
		MessageBoxA(0, "Error: WriteProcessMemory failed...", "D2RMulti", 0);
		CloseHandle(hProc);
		return;
	}

	std::cout << "[+] Loading kernel32.dll..." << std::endl;
	HMODULE hKernel32 = LoadLibraryA("kernel32");
	if (!hKernel32) {
		DWORD error = GetLastError();
		std::cout << "[!] LoadLibraryA failed with error: " << error << std::endl;
		MessageBoxA(0, "Error: Failed to load kernel32.dll...", "D2RMulti", 0);
		CloseHandle(hProc);
		return;
	}

	LPVOID lpStartAddress = GetProcAddress(hKernel32, "LoadLibraryW");
	if (!lpStartAddress) {
		DWORD error = GetLastError();
		std::cout << "[!] GetProcAddress failed with error: " << error << std::endl;
		MessageBoxA(0, "Error: Failed to get LoadLibraryW address...", "D2RMulti", 0);
		CloseHandle(hProc);
		return;
	}

	std::cout << "[+] Creating remote thread to load DLL..." << std::endl;
	HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(lpStartAddress), lpAlloc, 0, NULL);
	if (hThread == NULL) {
		DWORD error = GetLastError();
		std::cout << "[!] CreateRemoteThread failed with error: " << error << std::endl;
		MessageBoxA(0, "Error: CreateRemoteThread failed...", "D2RMulti", 0);
		CloseHandle(hProc);
		return;
	}

	WaitForSingleObject(hThread, INFINITE);
	DWORD exitCode;
	GetExitCodeThread(hThread, &exitCode);
	std::cout << "[+] Remote thread completed with exit code: " << exitCode << std::endl;

	CloseHandle(hProc);
	CloseHandle(hThread);
	std::wcout << L"[+] DLL injected successfully: " << wpath << std::endl;
}

void RemoveAllHandles() {
	auto fileExists = [](const std::string& path) -> bool {
		return std::filesystem::exists(path);
	};

	std::string path = GetExeDirectory();
	if (!fileExists(path + "handle64.exe")) {
		MessageBoxA(0, "Handle64.exe needs to be right next of this executable to run.", "D2RMulti", 0);
		return;
	}

	std::string CMD = "\"" + path + "handle64.exe\" - accepteula - a - p D2R";
	std::cout << "Handle Command: [" << CMD << "]" << std::endl;
	std::vector<std::string> commandOutput = RunCommandWithOutput(path + "handle64.exe -accepteula -a -p D2R");

	std::string proc_id_populated = "";
	std::string handle_id_populated = "";
	for (const std::string& line : commandOutput) {

		auto _D2R = line.find("D2R");
		auto _PID = line.find(" pid:");
		if (_D2R != std::string::npos && _PID != std::string::npos) {
			size_t start = _PID + 6;
			std::string ProcessID = line.substr(start, line.find(" ", start) - start);
			proc_id_populated = ProcessID;
		}

		auto HANDLEID = line.find(": Event");
		if (HANDLEID != std::string::npos && line.find("DiabloII Check For Other Instances") != std::string::npos) {
			size_t end = HANDLEID;
			if (end != std::string::npos) {
				handle_id_populated = line.substr(0, end);
			}
		}

		if (!handle_id_populated.empty()) {		

			std::string closeCommand = "\"" + path + "handle64.exe\" -p " + proc_id_populated + " -c " + handle_id_populated + " -y";
			std::cout << "Close D2R Instance Command: [" << closeCommand << "]" << std::endl;
			system(closeCommand.c_str());
			handle_id_populated = "";	// im paranoid
		}
	}
}
bool WaitForProcessReady(DWORD pid, DWORD timeoutMs) {
	DWORD startTime = GetTickCount();
	HANDLE hSnapshot;
	bool ready = false;

	std::cout << "[+] Waiting for process " << pid << " to be ready..." << std::endl;

	while (GetTickCount() - startTime < timeoutMs) {
		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			DWORD error = GetLastError();
			std::cout << "[!] Snapshot failed with error: " << error << std::endl;
			Sleep(100);
			continue;
		}

		MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
		if (Module32First(hSnapshot, &me32)) {
			std::wcout << L"[+] Main module loaded: " << me32.szModule << std::endl;
			ready = true;
		}
		CloseHandle(hSnapshot);

		if (ready) {
			break;
		}
		Sleep(100);
	}

	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (hProc) {
		DWORD exitCode;
		if (GetExitCodeProcess(hProc, &exitCode)) {
			if (exitCode == STILL_ACTIVE) {
				std::cout << "[+] Process " << pid << " is still active." << std::endl;
			}
			else {
				std::cout << "[!] Process " << pid << " terminated with code: " << exitCode << std::endl;
				ready = false;
			}
		}
		else {
			std::cout << "[!] GetExitCodeProcess failed: " << GetLastError() << std::endl;
			ready = false;
		}
		CloseHandle(hProc);
	}
	else {
		std::cout << "[!] OpenProcess failed: " << GetLastError() << std::endl;
		ready = false;
	}

	std::cout << "[+] WaitForProcessReady result: " << (ready ? "Ready" : "Not Ready") << std::endl;
	return ready;
}

bool SetWindowTitle(DWORD pid, const std::string& newTitle) {
	WindowTitleData data;
	data.pid = pid;
	data.wNewTitle = std::wstring(newTitle.begin(), newTitle.end());
	data.foundWindow = false;

	WNDENUMPROC enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
		WindowTitleData* dataPtr = reinterpret_cast<WindowTitleData*>(lParam);
		DWORD windowPid;
		GetWindowThreadProcessId(hwnd, &windowPid);

		if (windowPid == dataPtr->pid && IsWindowVisible(hwnd)) {
			SendMessageW(hwnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(dataPtr->wNewTitle.c_str()));
			dataPtr->foundWindow = true;
			return FALSE;
		}
		return TRUE;
	};
	for (int i = 0; i < 10 && !data.foundWindow; ++i) {
		EnumWindows(enumProc, reinterpret_cast<LPARAM>(&data));
		if (!data.foundWindow) {
			Sleep(200);
		}
	}
	return data.foundWindow;
}



DWORD FindProcessIdByWindowTitle(const std::string& titleToFind) {
	struct WindowProcessData {
		std::wstring wTargetTitle;
		DWORD pid = 0;
		bool found = false;
	};

	WindowProcessData data;
	data.wTargetTitle = std::wstring(titleToFind.begin(), titleToFind.end());

	WNDENUMPROC enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
		WindowProcessData* dataPtr = reinterpret_cast<WindowProcessData*>(lParam);

		wchar_t windowTitle[256];
		GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(wchar_t));

		if (IsWindowVisible(hwnd) && wcscmp(windowTitle, dataPtr->wTargetTitle.c_str()) == 0) {
			GetWindowThreadProcessId(hwnd, &dataPtr->pid);
			dataPtr->found = true;
			return FALSE;
		}
		return TRUE;
	};

	EnumWindows(enumProc, reinterpret_cast<LPARAM>(&data));

	return data.found ? data.pid : 0;
}