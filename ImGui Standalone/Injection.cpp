#include "Injection.h"
#include "Tools.h"

#include <string>
#include <filesystem>
#include <psapi.h>
#include <thread>
#include <tlhelp32.h>
#include <wtsapi32.h>
#pragma comment(lib, "Wtsapi32.lib")

std::vector<std::string>	RunCommandWithOutput(const std::string& command) {
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
DWORD                       RunCommand(const std::string& command) {
	STARTUPINFOA si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcessA(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		MessageBoxA(0, "Error: CreateProcess failed ...", "D2RMulti", 0);
		return 0;
	}


	WaitForInputIdle(pi.hProcess, INFINITE);
	DWORD pid = pi.dwProcessId;
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return pid;
}

void						EjectDLL(const int& pid, const std::string& path) {
	std::wstring wpath(path.begin(), path.end());
	std::wstring dllName = std::filesystem::path(wpath).filename();

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hSnapshot == INVALID_HANDLE_VALUE) return;

	MODULEENTRY32 ModuleEntry32 = { 0 };
	ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
	HMODULE targetModule = NULL;

	if (Module32First(hSnapshot, &ModuleEntry32)) {
		do {
			if (_wcsicmp(ModuleEntry32.szModule, dllName.c_str()) == 0) {
				targetModule = ModuleEntry32.hModule;
				break;
			}
		} while (Module32Next(hSnapshot, &ModuleEntry32));
	}
	CloseHandle(hSnapshot);

	if (targetModule) {
		HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (!hProc) return;

		HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
		LPVOID lpStartAddress = GetProcAddress(hKernel32, "FreeLibrary");
		if (!lpStartAddress) {
			CloseHandle(hProc);
			return;
		}

		HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(lpStartAddress), targetModule, 0, NULL);
		if (hThread) {
			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
		}
		else {
			DWORD error = GetLastError();
			std::cerr << "CreateRemoteThread failed with error: " << error << std::endl;
		}
		CloseHandle(hProc);
	}
}
void						InjectDLL(const int& pid, const std::string& path) {
	if (!std::filesystem::exists(path)) {
		MessageBoxA(0, "Error: Couldn't find DLL...", "D2RMulti", 0);
		return;
	}

	std::wstring wpath = std::wstring(path.begin(), path.end());
	long dll_size = static_cast<long>((wpath.length() + 1) * sizeof(wchar_t));
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProc == NULL) {
		MessageBoxA(0, "Error: Failed to open target process...", "D2RMulti", 0);
		return;
	}

	LPVOID lpAlloc = VirtualAllocEx(hProc, NULL, dll_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (lpAlloc == NULL) {
		MessageBoxA(0, "Error: VirtualAllocEx failed...", "D2RMulti", 0);
		CloseHandle(hProc);
		return;
	}

	if (WriteProcessMemory(hProc, lpAlloc, wpath.c_str(), dll_size, 0) == 0) {
		MessageBoxA(0, "Error: WriteProcessMemory failed...", "D2RMulti", 0);
		CloseHandle(hProc);
		return;
	}

	HMODULE hKernel32 = LoadLibraryA("kernel32");
	LPVOID lpStartAddress = GetProcAddress(hKernel32, "LoadLibraryW");
	HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(lpStartAddress), lpAlloc, 0, NULL);
	if (hThread == NULL) {
		MessageBoxA(0, "Error: CreateRemoteThread failed...", "D2RMulti", 0);
		CloseHandle(hProc);
		return;
	}
	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hProc);
	CloseHandle(hThread);
}

void						RemoveAllHandles() {
	auto fileExists = [](const std::string& path) -> bool {
		return std::filesystem::exists(path);
	};

	std::string path = GetExeDirectory();
	if (!fileExists(path + "handle64.exe")) {
		MessageBoxA(0, "Handle64.exe needs to be right next of this executable to run.", "D2RMulti", 0);
		return;
	}

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
			std::string closeCommand = path + "handle64.exe -p " + proc_id_populated + " -c " + handle_id_populated + " -y";
			system(closeCommand.c_str());
			handle_id_populated = "";	// im paranoid
		}
	}
}