#include <Windows.h>
#include <tchar.h>
#include "d3d11.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

#include "Main.h"
#include "Tools.h"
#include "Console.h"
#include "Registry.h"

// Original imgui standalone d3d11 source
// https://github.com/adamhlt/ImGui-Standalone
// thank you good sir


constexpr wchar_t WindowName[] = L"D2R Multiclient";

extern IMGUI_IMPL_API LRESULT       ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
class UI {
private:
    static ID3D11Device*            pd3dDevice;
    static ID3D11DeviceContext*     pd3dDeviceContext;
    static IDXGISwapChain*          pSwapChain;
    static ID3D11RenderTargetView*  pMainRenderTargetView;

    static bool                     CreateDeviceD3D(HWND hWnd);
    static void                     CleanupDeviceD3D();
    static void                     CreateRenderTarget();
    static void                     CleanupRenderTarget();
    static LRESULT WINAPI           WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
    static HMODULE                  hCurrentModule;
    static void                     Render();
};

ID3D11Device*                       UI::pd3dDevice = nullptr;
ID3D11DeviceContext*                UI::pd3dDeviceContext = nullptr;
IDXGISwapChain*                     UI::pSwapChain = nullptr;
ID3D11RenderTargetView*             UI::pMainRenderTargetView = nullptr;
HMODULE                             UI::hCurrentModule = nullptr;

int WINAPI                          wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
    
    if (!IsRunningAsAdmin()) {
        MessageBoxA(0, "Administrator privileges are required to run this process.", "D2RMulti", MB_OK | MB_ICONEXCLAMATION);
        return -1;
    }

    UI::Render();
    return 0;
}
bool                                UI::CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const UINT createDeviceFlags = 0;
    
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}
void                                UI::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer != nullptr)
    {
        pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pMainRenderTargetView);
        pBackBuffer->Release();
    }
}
void                                UI::CleanupRenderTarget() {
    if (pMainRenderTargetView)
    {
        pMainRenderTargetView->Release();
        pMainRenderTargetView = nullptr;
    }
}
void                                UI::CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (pSwapChain) {
        pSwapChain->Release();
        pSwapChain = nullptr;
    }

    if (pd3dDeviceContext) {
        pd3dDeviceContext->Release();
        pd3dDeviceContext = nullptr;
    }

    if (pd3dDevice) {
        pd3dDevice->Release();
        pd3dDevice = nullptr;
    }
}
LRESULT WINAPI                      UI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    switch (msg) {
    case WM_SIZE:
        if (pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) {
            return 0;
        }
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports) {
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;

    default:break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
void                                UI::Render() {
    ImGui_ImplWin32_EnableDpiAwareness();
    const WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, WindowName, nullptr };
    ::RegisterClassEx(&wc);
    const HWND hwnd = ::CreateWindow(wc.lpszClassName, WindowName, WS_OVERLAPPEDWINDOW, 100, 100, 50, 50, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return;
    }

    ::ShowWindow(hwnd, SW_HIDE);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    //ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Keeping the rounded corners as is
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.TabRounding = 5.0f;

    // Base palette (unchanged except where noted)
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f); // Dark base stays
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.07f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Textbox background—brighter and more distinct
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f); // Lighter than WindowBg for contrast
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f); // Clear hover
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.40f, 1.00f); // Noticeable active state

    // Rest of the unchanged base
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.02f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);

    // Accent elements
    colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.70f, 0.70f, 1.00f);

    // Table headers—more distinct from the background
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f); // Dark but noticeably lighter than WindowBg
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.38f, 1.00f); // Stronger hover contrast
    colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f); // Clear active selection

    // Rest of the style
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.20f, 0.60f, 0.60f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.60f, 0.60f, 0.65f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.20f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.20f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.30f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.60f, 0.60f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.20f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.20f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.95f, 0.95f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.15f, 0.15f, 0.18f, 0.50f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.15f, 0.15f, 0.18f, 0.50f);

    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
  
    ImFontConfig cfg;
    cfg.SizePixels = 13;
    ImGui::GetIO().Fonts->AddFontDefault(&cfg);
    
    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext);

    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    static bool Init;

    GetSettings->LoadConfig();
    if (ShowConsole) { SpawnConsole_(); }
    while (isRunning) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {

            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                break;
            }
            else {
                HandleKeyPress(msg.message, msg.wParam);
            }


        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        if (!Init) {
            SetNextWindowsInTheMiddle(450, 200);
            Init = true;
        }
        Main();

        ImGui::EndFrame();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        pd3dDeviceContext->OMSetRenderTargets(1, &pMainRenderTargetView, nullptr);
        pd3dDeviceContext->ClearRenderTargetView(pMainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        pSwapChain->Present(1, 0);

    }

    if (ShowConsole) { FreeConsole_(); }

    // Close
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    #ifdef _WINDLL
    CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)FreeLibrary, hCurrentModule, NULL, nullptr);
    #endif
}