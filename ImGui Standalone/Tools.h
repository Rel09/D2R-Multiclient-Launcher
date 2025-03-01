#pragma once

#include "ImGui/imgui.h"

#include <vector>
#include <iostream>

std::string					GetExeDirectory();
bool						IsRunningAsAdmin();
bool						IsProcessRunning();
std::vector<std::string>	ReadFile(const std::string& filename);
void						SetNextWindowsInTheMiddle(const int WinSizeX, const int WinSizeY);