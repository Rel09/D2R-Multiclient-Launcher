#include "Main.h"
#include "ImGui/imgui.h"

#include "Menu.h"
#include "Tools.h"
#include "Timer.h"
#include "Registry.h"

#include <windows.h>

std::vector<D2RInstanceStruct>      Data = {};                          // DB
bool                                isRunning   = true;                 // This shutdown the process, extern in .h, used in __init__
bool                                ShowConsole = false;                // Debugging
constexpr const char                Appname[] = "D2R Multiclient";

void Main() {
    static TIMESTAMP CheckD2RInstanceTimer = Timer::InitTimer();
    static bool Init;

    // Spawn in the middle of the screen
    if (!Init) {
        SetNextWindowsInTheMiddle(450, 200);
        Init = true;
    }

    if (ImGui::Begin(Appname, &isRunning, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar)) {

        // Top bar
        MenuRoutine();

        // Scan Processes every 2.5 seconds
        if (Timer::Sleep(CheckD2RInstanceTimer, 2500)) {

            // Check if Process are still running
            for (auto& i : Data) {
                auto isProcessRunning = [](DWORD pid) -> bool {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                    if (!hProcess) return false;

                    DWORD exitCode;
                    bool running = GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE;
                    CloseHandle(hProcess);
                    return running;
                };
                bool isD2RRunning = isProcessRunning(i.ProcessID);
                if (i.ProcessID != -1 && !isD2RRunning) {
                    i.ProcessID = -1;
                }
            }


        }

        // ++++++++++++ Table ++++++++++++
        const std::vector<std::string> headers = { "#", "Profile Name", "Realm", "Mods" }; // "DLL Path"
        ImGui::BeginTable("table1", static_cast<int>(headers.size()), ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg );

        // Setup headers and columns
        ImGui::TableSetupColumn(headers[0].c_str(), ImGuiTableColumnFlags_WidthFixed, 10.0f);  // "#"
        for (uint8_t index = 1; index < headers.size(); ++index) {
            ImGui::TableSetupColumn(headers[index].c_str());
        }
        ImGui::TableHeadersRow();

        // List Item
        for (auto& i : Data) {
            ImGui::TableNextRow();
            ImGui::PushID(&i);

            // "*"
            ImGui::TableNextColumn();
            ImColor Temp = i.ProcessID == -1 ? ImColor(255, 0, 0) : ImColor(0, 255, 0);
            ImGui::TextColored(Temp, "*");

            // __ Selected Row __
            ImGui::TableNextColumn();
            if (ImGui::Selectable(i.Name.c_str(), i.isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                i.isSelected = !i.isSelected;
            }

            // Realm
            ImGui::TableNextColumn();
            ImGui::Text("%s", i.Realm.c_str());

            //// DLL Path
            //ImGui::TableNextColumn();
            //ImGui::Text("%s", i.DllPath.c_str());

            // Mods
            ImGui::TableNextColumn();
            ImGui::Text("%s", i.ModList.c_str());

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}
