#pragma once

#include <iostream>
#include <Windows.h>

#define GetSettings Settings::Get()
#define REGISTRY_PATH "SOFTWARE\\D2R_MULTI"
#define CSVFILENAME "output.txt"

class Settings {
public:
    void LoadConfig();
    void SaveConfig();
    void UpdateConfig();

    static Settings* Get() {
        static Settings T;
        return &T;
    }

    void ExportConfigToCSV();
    void ImportConfigFromCSV();

private:

    void DeleteAllConfigs();
    void DeleteRegistryKey(HKEY hParentKey, const std::string& subKey);
    Settings();
};
