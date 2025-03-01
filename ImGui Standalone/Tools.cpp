#include "Tools.h"

#include <Windows.h>
#include <fstream>
#include <tlhelp32.h>
#include <filesystem>

bool IsRunningAsAdmin() {
	BOOL isAdmin = FALSE;
	HANDLE token = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
		TOKEN_ELEVATION elevation;
		DWORD size;
		if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
			isAdmin = elevation.TokenIsElevated;
		}
		CloseHandle(token);
	}
	return isAdmin;
}
std::string	GetExeDirectory() {
	char path[MAX_PATH];
	GetModuleFileNameA(NULL, path, MAX_PATH);
	std::filesystem::path exePath = path;
	return exePath.parent_path().string() + '\\';
}

void SetNextWindowsInTheMiddle(const int WinSizeX, const int WinSizeY) {
	ImVec2 windowSize(static_cast<float>(WinSizeX), static_cast<float>(WinSizeY));
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	ImVec2 screenSize((float)screenWidth, (float)screenHeight);
	ImVec2 windowPos = ImVec2((screenSize.x - windowSize.x) * 0.5f, (screenSize.y - windowSize.y) * 0.5f);
	ImGui::SetNextWindowPos(windowPos);
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WinSizeX), static_cast<float>(WinSizeY)));
}

std::vector<std::string> ReadFile(const std::string& filename) {
	std::ifstream file(filename);
	std::vector<std::string> lines;
	std::string line;

	while (std::getline(file, line)) {
		lines.push_back(line);
	}

	return lines;
}

bool IsProcessRunning() {
    wchar_t exeName[MAX_PATH];
    if (!GetModuleFileNameW(NULL, exeName, MAX_PATH)) {
        return false;
    }

    wchar_t* exeBaseName = wcsrchr(exeName, L'\\');
    exeBaseName = exeBaseName ? exeBaseName + 1 : exeName;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    int instanceCount = 0;
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (wcscmp(pe.szExeFile, exeBaseName) == 0 && pe.th32ProcessID != GetCurrentProcessId()) {
                if (++instanceCount > 0) {
                    CloseHandle(hSnapshot);
                    return true;
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return false;
}