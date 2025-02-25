#include "Tools.h"

#include <Windows.h>
#include <fstream>
#include <filesystem>

bool		IsRunningAsAdmin() {
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

void		SetNextWindowsInTheMiddle(const int WinSizeX, const int WinSizeY) {
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