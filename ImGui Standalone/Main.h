#pragma once

#include <vector>
#include <string>

struct D2RInstanceStruct {
    std::string Name;
    std::string Email;
    std::string Password;
    std::string D2REXEPath;
    std::string DllPath;
    std::string ModList;
    std::string Realm;

    // ____ Needed ____
    int         ProcessID = -1;
    bool        isSelected = false;
    bool        isBeingEdited = false;
};

extern std::vector<D2RInstanceStruct>   Data;
extern bool                             isRunning;
extern bool                             ShowConsole;

void Main();