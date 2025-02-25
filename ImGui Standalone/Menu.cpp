#include "Menu.h"
#include "ImGui/imgui.h"

#include "Main.h"
#include "Tools.h"
#include "Registry.h"
#include "Injection.h"

#include <Windows.h>
#include <filesystem>
#include <chrono>

static bool AddWindows = false;

static void DisplayTopBar() {
    if (ImGui::BeginMenuBar()) {

        // Actions Menu
        if (ImGui::BeginMenu("Actions")) {
            if (ImGui::MenuItem("Add")) {
                AddWindows = true;
            }
            if (ImGui::MenuItem("Edit")) {
                for (auto& i : Data) {
                    if (i.isSelected) {
                        i.isBeingEdited = true;
                    }
                }
            }
            if (ImGui::MenuItem("Delete")) {

                bool DeletedSomething = false;
                for (auto it = Data.begin(); it != Data.end();) {
                    auto& i = *it;

                    if (i.isSelected) {

                        std::string NAME = "Do you want to delete [ " + i.Name + " ] ?";
                        if (MessageBoxA(0, NAME.c_str(), "D2RMulti", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                            it = Data.erase(it);
                            DeletedSomething = true;
                        }
                        else {
                            ++it;
                        }
                    }
                    else {
                        ++it;
                    }
                }
                if (DeletedSomething) {
                    GetSettings->UpdateConfig();
                }

            }
            if (ImGui::MenuItem("Copy")) {
                for (auto& i : Data) {

                    bool newChange = false;
                    if (i.isSelected) {

                        std::string NAME = "Do you want to copy [ " + i.Name + " ] ?";
                        if (MessageBoxA(0, NAME.c_str(), "D2RMulti", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                            D2RInstanceStruct T = i;
                            auto now = std::chrono::system_clock::now();
                            auto time_since_epoch = now.time_since_epoch();
                            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
                            T.Name = T.Name + "_copy_" + std::to_string(millis);
                            Data.push_back(T);
                            newChange = true;
                        }
                
                    }
                    if (newChange) {
                        GetSettings->SaveConfig();
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Start")) {

                if (Data.size() == 0) {
                    MessageBoxA(0, "No selection made. Starting is not possible.", "D2RMulti", 0);
                }

                else {
                    for (auto& i : Data) {
                        if (i.isSelected) {
     
                            std::string TempStr = i.D2REXEPath + " -username " + i.Email + " -password " + i.Password + " -address " + i.Realm + ".actual.battle.net";
                            if (!i.ModList.empty()) {
                                TempStr += " -mod " + i.ModList;
                            }

                            // Scan all Running executable thats starts with D2R to remove handle
                            RemoveAllHandles();

                            // Start the Process
                            i.ProcessID = RunCommand(TempStr);;

                            // Inject if a Path is given
                            if (i.ProcessID != 0 && !i.DllPath.empty()) {
                                Sleep(1500);
                                InjectDLL(i.ProcessID, i.DllPath);
                            }
             
                        }
                    }
                }

               // StartWindows = true;
            }    
            if (ImGui::MenuItem("Stop")) {

                for (auto& i : Data) {
                    if (i.ProcessID != -1) {
                        auto closeProcessByID = [](DWORD pid) -> bool {
                            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                            if (hProcess == nullptr) {
                                std::cerr << "Failed to open process with ID: " << pid << std::endl;
                                return false;
                            }

                            BOOL result = TerminateProcess(hProcess, 0);
                            CloseHandle(hProcess);

                            if (result) {
                                std::cout << "Successfully terminated process with ID: " << pid << std::endl;
                                return true;
                            }
                            else {
                                std::cerr << "Failed to terminate process with ID: " << pid << std::endl;
                                return false;
                            }
                            };
                        closeProcessByID(i.ProcessID);
                        i.ProcessID = -1;
                    }
                }

            }
            ImGui::Separator();
            if (ImGui::MenuItem("Import")) {

                auto fileExists = [](const std::string& path) -> bool {
                    return std::filesystem::exists(path);
                    };

                std::string filePath = GetExeDirectory() + "config.csv";

                if (!fileExists(filePath)) {
                    MessageBoxA(0, "Rename your file to 'config.csv' and copy it next to this executable", "D2RMulti", 0);
                }
                else {
                    int response = MessageBoxA(0, "Are you sure you want to import the configuration?", "D2RMulti", MB_OKCANCEL | MB_ICONQUESTION);
                    if (response == IDOK) {
                        GetSettings->ImportConfigFromCSV();
                    }
                }
            }
            if (ImGui::MenuItem("Export")) {

                GetSettings->ExportConfigToCSV();

            }
            ImGui::Separator();
            if (ImGui::MenuItem("Inject Dll")) {

                for (auto& i : Data) {
                    if (i.isSelected && i.ProcessID != -1) {
                        InjectDLL(i.ProcessID, i.DllPath);
                    }
                }

            }          
            if (ImGui::MenuItem("Eject Dll")) {

                for (auto& i : Data) {
                    if (i.isSelected && i.ProcessID != -1) {
                        EjectDLL(i.ProcessID, i.DllPath);
                    }
                }

            }
            ImGui::EndMenu();
        }

        // Help Menu
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                MessageBoxA(0, "Created by Religious09 - 192.168.0.8 @ Discord\nYou are respecting EULA, usage of 3rd party tools is at your own risk", "About", 0);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

static void DisplayExternalMenu() {
    auto InputTextWithResize = [](const char* label, std::string& buffer, size_t maxSize, ImGuiInputTextFlags flags = 0) {
        char tempBuffer[MAX_PATH + 1] = { 0 };
        strncpy_s(tempBuffer, buffer.c_str(), maxSize);
        if (ImGui::InputText(label, tempBuffer, maxSize, flags)) {
            buffer = tempBuffer;
            if (buffer.size() > maxSize) {
                buffer.resize(maxSize);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                                         ADD WINDOWS
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (AddWindows) {
        ImGui::Begin("Add", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);          // Start

        static D2RInstanceStruct TempStruct;

        // User Inputs
        InputTextWithResize("Profile Name", TempStruct.Name, MAX_PATH);
        ImGui::Separator();

        InputTextWithResize("B.net Email", TempStruct.Email, MAX_PATH);
        ImGui::Separator();

        InputTextWithResize("Password", TempStruct.Password, MAX_PATH, ImGuiInputTextFlags_Password);
        ImGui::Separator();

        InputTextWithResize("Dll Path   [Optional]", TempStruct.DllPath, MAX_PATH);
        ImGui::Separator();

        InputTextWithResize("Mods       [Optional]", TempStruct.ModList, MAX_PATH);
        ImGui::Separator();

        InputTextWithResize("Realm      [us, eu, asia]", TempStruct.Realm, 4);
        ImGui::Separator();

        InputTextWithResize("D2R Path", TempStruct.D2REXEPath, MAX_PATH);
        ImGui::Text("Make sure your D2R executable starts with D2R");
        ImGui::Text("Ex: 'D2R_001.exe', 'D2R.exe'");

        ImGui::Separator();


        // __________________ Buttons __________________
        auto resetdata = [](D2RInstanceStruct& instance) {
            instance.Name.clear();
            instance.Email.clear();
            instance.Password.clear();
            instance.D2REXEPath.clear();
            instance.DllPath.clear();
            instance.ModList.clear();
            instance.Realm.clear();
        };

        // Save Button
        if (ImGui::Button("Save", { 70, 30 })) {
            bool canAdd = true;

            // Can do a bunch of filtering here...

            // Add if filters are good
            if (canAdd) {
                Data.push_back(TempStruct);
                GetSettings->UpdateConfig();
                resetdata(TempStruct);
                AddWindows = false;
            }
        }

        ImGui::SameLine();

        // Cancel Button
        if (ImGui::Button("Cancel", { 70, 30 })) {
            resetdata(TempStruct);
            AddWindows = false;
        }

        ImGui::End();                                                                                       // End
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //                                                         EDIT WINDOWS
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    for (auto& i : Data) {
        if (i.isBeingEdited) {
            std::string windowName = "Edit###" + std::to_string(&i - &Data[0]);
            ImGui::Begin(windowName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

            // User Inputs
            InputTextWithResize("Profile Name", i.Name, MAX_PATH);
            ImGui::Separator();

            InputTextWithResize("B.net Email", i.Email, MAX_PATH);
            ImGui::Separator();

            InputTextWithResize("Password", i.Password, MAX_PATH, ImGuiInputTextFlags_Password);
            ImGui::Separator();

            InputTextWithResize("Dll Path   [Optional]", i.DllPath, MAX_PATH);
            ImGui::Separator();

            InputTextWithResize("Mods       [Optional]", i.ModList, MAX_PATH);
            ImGui::Separator();

            InputTextWithResize("Realm      [us, eu, asia]", i.Realm, 5);
            ImGui::Separator();

            InputTextWithResize("D2R Path", i.D2REXEPath, MAX_PATH);
            ImGui::Text("Make sure your D2R executable starts with D2R");
            ImGui::Text("Ex: 'D2R_001.exe', 'D2R.exe'");

            ImGui::Separator();

            if (ImGui::Button("Close", {50, 20})) {
                GetSettings->UpdateConfig();
                i.isBeingEdited = false;
            }

            ImGui::End();
        }
    }
    
}

void MenuRoutine() {
    DisplayTopBar();
    DisplayExternalMenu();
}