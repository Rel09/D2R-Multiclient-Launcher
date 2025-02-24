
#include "Registry.h"
#include "Main.h"
#include "Tools.h"

#include <fstream>
#include <sstream>

Settings::Settings() {}

static bool         ReadRegistryValue(HKEY hKey, const std::string& valueName, std::string& outValue) {
    DWORD type;
    BYTE data[256];
    DWORD dataSize = sizeof(data);
    if (RegQueryValueExA(hKey, valueName.c_str(), NULL, &type, data, &dataSize) == ERROR_SUCCESS && type == REG_SZ) {
        outValue = std::string((char*)data, dataSize - 1);
        return true;
    }
    return false;
}

void                Settings::SaveConfig() {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REGISTRY_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        MessageBoxA(0, "Error: Couldn't save Registry key.\nThis Process uses the Registry 'HKEY_CURRENT_USER\\SOFTWARE\\D2R_MULTI'", "9R", 0);
        return;
    }

    for (const auto& instance : Data) {
        HKEY hInstanceKey;
        if (RegCreateKeyExA(hKey, instance.Name.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hInstanceKey, NULL) == ERROR_SUCCESS) {

            RegSetValueExA(hInstanceKey, "Name", 0, REG_SZ, (BYTE*)instance.Name.c_str(), static_cast<DWORD>(instance.Name.size() + 1));
            RegSetValueExA(hInstanceKey, "Email", 0, REG_SZ, (BYTE*)instance.Email.c_str(), static_cast<DWORD>(instance.Email.size() + 1));
            RegSetValueExA(hInstanceKey, "Password", 0, REG_SZ, (BYTE*)instance.Password.c_str(), static_cast<DWORD>(instance.Password.size() + 1));
            RegSetValueExA(hInstanceKey, "D2REXEPath", 0, REG_SZ, (BYTE*)instance.D2REXEPath.c_str(), static_cast<DWORD>(instance.D2REXEPath.size() + 1));
            RegSetValueExA(hInstanceKey, "DllPath", 0, REG_SZ, (BYTE*)instance.DllPath.c_str(), static_cast<DWORD>(instance.DllPath.size() + 1));
            RegSetValueExA(hInstanceKey, "ModList", 0, REG_SZ, (BYTE*)instance.ModList.c_str(), static_cast<DWORD>(instance.ModList.size() + 1));
            RegSetValueExA(hInstanceKey, "Realm", 0, REG_SZ, (BYTE*)instance.Realm.c_str(), static_cast<DWORD>(instance.Realm.size() + 1));

            RegCloseKey(hInstanceKey);
        }
    }

    RegCloseKey(hKey);
}
void                Settings::LoadConfig() {
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_PATH, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        std::cout << "Failed to open registry key: " << REGISTRY_PATH << std::endl;
        return;
    }

    DWORD index = 0;
    char subKeyName[256];
    DWORD subKeyNameSize;

    while (RegEnumKeyExA(hKey, index, subKeyName, &(subKeyNameSize = sizeof(subKeyName)), NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        std::string instanceName(subKeyName);
        HKEY hInstanceKey;
        if (RegOpenKeyExA(hKey, subKeyName, 0, KEY_READ, &hInstanceKey) == ERROR_SUCCESS) {
            D2RInstanceStruct instance;

            // Read each field from the subkey
            ReadRegistryValue(hInstanceKey, "Name",         instance.Name);
            ReadRegistryValue(hInstanceKey, "Email",        instance.Email);
            ReadRegistryValue(hInstanceKey, "Password",     instance.Password);
            ReadRegistryValue(hInstanceKey, "D2REXEPath",   instance.D2REXEPath);
            ReadRegistryValue(hInstanceKey, "DllPath",      instance.DllPath);
            ReadRegistryValue(hInstanceKey, "ModList",      instance.ModList);
            ReadRegistryValue(hInstanceKey, "Realm",        instance.Realm);

            Data.push_back(instance);
            RegCloseKey(hInstanceKey);
        }
        index++;
    }

    RegCloseKey(hKey);
}
void                Settings::UpdateConfig() {
    DeleteAllConfigs();
    SaveConfig();
}

void                Settings::DeleteRegistryKey(HKEY hParentKey, const std::string& subKey) {
    HKEY hSubKey;
    if (RegOpenKeyExA(hParentKey, subKey.c_str(), 0, KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS) {
        // Recursively delete all subkeys
        char childKey[256];
        DWORD childKeySize;

        while (true) {
            childKeySize = sizeof(childKey);
            LONG result = RegEnumKeyExA(hSubKey, 0, childKey, &childKeySize, NULL, NULL, NULL, NULL);
            if (result == ERROR_SUCCESS) {
                DeleteRegistryKey(hSubKey, childKey);
            }
            else if (result == ERROR_NO_MORE_ITEMS) {
                break;
            }
            else {
                std::cout << "Error enumerating subkeys: " << GetLastError() << std::endl;
                break;
            }
        }

        RegCloseKey(hSubKey);
    }

    // Now delete the key itself
    if (RegDeleteKeyA(hParentKey, subKey.c_str()) == ERROR_SUCCESS) {
        std::cout << "Successfully deleted subkey: " << subKey << std::endl;
    }
    else {
        std::cout << "Failed to delete subkey: " << subKey << " with error code: " << GetLastError() << std::endl;
    }
}
void                Settings::DeleteAllConfigs() {
    HKEY hKey;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_PATH, 0, KEY_ALL_ACCESS | KEY_ENUMERATE_SUB_KEYS, &hKey) != ERROR_SUCCESS) {
        std::cout << "Error: Couldn't open the registry path " << REGISTRY_PATH << " with error code: " << GetLastError() << std::endl;
        return;
    }

    char subKeyName[256];
    DWORD subKeyNameSize;

    while (true) {
        subKeyNameSize = sizeof(subKeyName);
        LONG result = RegEnumKeyExA(hKey, 0, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL);

        if (result == ERROR_SUCCESS) {
            DeleteRegistryKey(hKey, subKeyName);
        }
        else if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }
        else {
            std::cout << "Error enumerating subkeys: " << GetLastError() << std::endl;
            break;
        }
    }

    RegCloseKey(hKey);

    if (RegDeleteKeyA(HKEY_CURRENT_USER, REGISTRY_PATH) == ERROR_SUCCESS) {
        std::cout << "Successfully deleted registry root key: " << REGISTRY_PATH << std::endl;
    }
    else {
        std::cout << "Failed to delete registry root key: " << REGISTRY_PATH << " with error code: " << GetLastError() << std::endl;
    }
}

void                Settings::ExportConfigToCSV() {
    std::string filename = GetExeDirectory() + "config.csv";
    std::ofstream file(filename);
    if (!file.is_open()) {
        MessageBoxA(0, ("Failed to open file for writing: " + filename).c_str(), "9R", 0);
        std::cout << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    const std::string delimiter = " ||| ";

    for (const auto& instance : Data) {
        file << instance.Name << delimiter
            << instance.Email << delimiter
            << instance.Password << delimiter
            << instance.D2REXEPath << delimiter
            << instance.DllPath << delimiter
            << instance.ModList << delimiter
            << instance.Realm << std::endl;
    }

    file.close();
    std::cout << "Configuration exported to: " << filename << std::endl;
    MessageBoxA(0, ("Configuration exported to: " + filename).c_str(), "9R", 0);
}
void                Settings::ImportConfigFromCSV() {
    std::string filename = GetExeDirectory() + "config.csv";
    std::ifstream file(filename);
    if (!file.is_open()) {
        MessageBoxA(0, ("Failed to open file for reading: " + filename).c_str(), "9R", 0);
        std::cout << "Failed to open file for reading: " << filename << std::endl;
        return;
    }

    std::cout << "Importing configuration from: " << filename << std::endl;

    std::string line;
    Data.clear();
    const std::string delimiter = " ||| ";

    while (std::getline(file, line)) {
        size_t pos = 0;
        D2RInstanceStruct instance;
        size_t start = 0, end;

        end = line.find(delimiter, start);
        instance.Name = line.substr(start, end - start);

        start = end + delimiter.length();
        end = line.find(delimiter, start);
        instance.Email = line.substr(start, end - start);

        start = end + delimiter.length();
        end = line.find(delimiter, start);
        instance.Password = line.substr(start, end - start);

        start = end + delimiter.length();
        end = line.find(delimiter, start);
        instance.D2REXEPath = line.substr(start, end - start);

        start = end + delimiter.length();
        end = line.find(delimiter, start);
        instance.DllPath = line.substr(start, end - start);

        start = end + delimiter.length();
        end = line.find(delimiter, start);
        instance.ModList = line.substr(start, end - start);

        start = end + delimiter.length();
        instance.Realm = line.substr(start);

        Data.push_back(instance);
    }

    file.close();
    UpdateConfig();
    std::cout << "Configuration imported successfully from: " << filename << std::endl;
    MessageBoxA(0, "Configuration imported successfully", "9R", 0);
}


