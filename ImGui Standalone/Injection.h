#pragma once

#include <vector>
#include <iostream>
#include <Windows.h>

void RemoveAllHandles();
void EjectDLL(int pid, const std::string& dllName);
void InjectDLL(const int& pid, const std::string& path);
DWORD RunCommand(const std::string& command);
std::vector<std::string> RunCommandWithOutput(const std::string& command);
bool WaitForProcessReady(DWORD pid, DWORD timeoutMs = 5000);
bool SetWindowTitle(DWORD pid, const std::string& newTitle);
DWORD FindProcessIdByWindowTitle(const std::string& titleToFind);