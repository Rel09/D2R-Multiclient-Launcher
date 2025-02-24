#pragma once

#include <vector>
#include <iostream>
#include <Windows.h>

void						RemoveAllHandles();
void						EjectDLL(const int& pid, const std::string& path);
void						InjectDLL(const int& pid, const std::string& path);
DWORD                       RunCommand(const std::string& command);
std::vector<std::string>	RunCommandWithOutput(const std::string& command);