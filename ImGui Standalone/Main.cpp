#include "Main.h"
#include "ImGui/imgui.h"

#include "Menu.h"
#include "Tools.h"
#include "Timer.h"
#include "Registry.h"

std::vector<D2RInstanceStruct>      Data = {};
bool                                isRunning   = true;
bool                                ShowConsole = false;
constexpr int                       ProcessPingTimer = 8000;
constexpr const char                Appname[] = "D2R Multiclient";

void Main() {
    static std::string nameVer = std::string(Appname) + " [ v" + VERSION + " ]";
    if (ImGui::Begin(nameVer.c_str(), &isRunning, ImGuiWindowFlags_MenuBar)) {

        // Top bar
        MenuRoutine();

        // Scan Running (green *) Processes every X seconds
        static TIMESTAMP CheckD2RInstanceTimer = Timer::InitTimer();
        if (Timer::Sleep(CheckD2RInstanceTimer, ProcessPingTimer)) {

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
        const std::vector<std::string> headers = { " ", "Profile Name", "Realm", "Mods" };
        ImGui::BeginTable("table1", static_cast<int>(headers.size()), ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);

        // Setup headers and columns
        ImGui::TableSetupColumn(headers[0].c_str(), ImGuiTableColumnFlags_WidthFixed, 10.0f);
        for (uint8_t index = 1; index < headers.size(); ++index) {
            ImGui::TableSetupColumn(headers[index].c_str());
        }
        ImGui::TableHeadersRow();

        // List Item
        for (auto& i : Data) {
            ImGui::TableNextRow();
            ImGui::PushID(&i);

            // >
            ImGui::TableNextColumn();
            const float size = 8.0f;
            const ImVec2 cellMin = ImGui::GetCursorScreenPos();
            const ImVec2 center = ImVec2(cellMin.x + (ImGui::GetColumnWidth() * 0.5f), cellMin.y + (ImGui::GetTextLineHeight() * 0.5f));
            ImVec2 p1 = ImVec2(center.x - size * 0.5f, center.y - size * 0.5f);
            ImVec2 p2 = ImVec2(center.x - size * 0.5f, center.y + size * 0.5f);
            ImVec2 p3 = ImVec2(center.x + size * 0.5f, center.y);
            ImGui::GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, i.ProcessID == -1 ? ImColor(150, 150, 150) : ImColor(0, 255, 0));
            ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));


            // __ Selected Row __
            ImGui::TableNextColumn();
            if (ImGui::Selectable(i.Name.c_str(), i.isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                i.isSelected = !i.isSelected;
            }

            // Realm
            ImGui::TableNextColumn();
            ImGui::Text("%s", i.Realm.c_str());

            // Mods
            ImGui::TableNextColumn();
            ImGui::Text("%s", i.ModList.c_str());

            ImGui::PopID();

        }
        ImGui::EndTable();
        ImGui::End();
    }
}

void HandleKeyPress(const UINT& message, const WPARAM& key) {
    switch (message) {

        //case WM_KEYDOWN: {

        //    break;
        //}

        //case WM_SYSKEYDOWN: {

        //    break;
        //}

        // Key up
        case WM_KEYUP: {

            switch (key) {

                // INS -> START
                case VK_INSERT: {
                    StartSelected();
                    break;
                }

                // Del -> STOP
                case VK_DELETE: {
                    StopSelected();
                    break;
                }

                // Home -> INJECT
                case VK_HOME: {
                    InjectSelected();
                    break;
                }

                // End - > EJECT
                case VK_END: {
                    EjectSelected();
                    break;
                }

                default: break;
            }
            break;
        }

        default: break;
    }
}