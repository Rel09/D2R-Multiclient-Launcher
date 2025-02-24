#include "Console.h"

#include <windows.h>
#include <iostream>
#include <fstream>

void SpawnConsole_() {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);
}

void FreeConsole_() {
    fclose(stdout);
    fclose(stderr);
    fclose(stdin);
    FreeConsole();
}