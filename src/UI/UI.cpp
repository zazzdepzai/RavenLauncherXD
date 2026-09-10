#include "UI.h"
#include "../DiscordRPC.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "embedded_assets.h"
#include <d3d11.h>
#include <windows.h>
#include <string>
#include <cstdio>

// ---- DX11 device (shared from main.cpp) ----
extern ID3D11Device*           g_pd3dDevice;
extern ID3D11DeviceContext*    g_pd3dDeviceContext;
ID3D11ShaderResourceView*      g_logoTex = nullptr;
static bool                    g_logoTried = false;

extern void LoadTextureFromMemory(const unsigned char* data, size_t len, ID3D11ShaderResourceView** out);

// --------- Palette ---------
static ImVec4 C(float r, float g, float b, float a=1.f){ return ImVec4(r,g,b,a); }
static ImU32 U32(int r, int g, int b, int a=255){ return IM_COL32(r,g,b,a); }

static const ImU32 COL_BG        = U32(10, 16, 32);
static const ImU32 COL_PANEL     = U32(20, 28, 51);
static const ImU32 COL_CARD      = U32(28, 38, 66);
static const ImU32 COL_CARD_HOV  = U32(36, 48, 82);
static const ImU32 COL_ACCENT    = U32(91, 141, 214);
static const ImU32 COL_ACCENT2   = U32(123, 167, 224);
static const ImU32 COL_TEXT      = U32(232, 238, 248);
static const ImU32 COL_TEXT_DIM  = U32(150, 165, 195);

// --------- Style ---------
void UI::ApplyStyle() {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding      = 18.f;
    st.ChildRounding       = 14.f;
    st.FrameRounding       = 10.f;
    st.PopupRounding       = 14.f;
    st.ScrollbarRounding   = 12.f;
    st.GrabRounding        = 12.f;
    st.TabRounding         = 10.f;
    st.WindowBorderSize    = 0.f;
    st.FrameBorderSize     = 0.f;
    st.WindowPadding       = ImVec2(0,0);
    st.FramePadding        = ImVec2(14, 10);
    st.ItemSpacing         = ImVec2(10, 10);
    st.ItemInnerSpacing    = ImVec2(8, 6);

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg]            = C(0.04f,0.06f,0.13f,1.f);
    c[ImGuiCol_ChildBg]             = C(0.08f,0.11f,0.20f,1.f);
    c[ImGuiCol_PopupBg]             = C(0.08f,0.11f,0.20f,1.f);
    c[ImGuiCol_Border]              = C(0.20f,0.30f,0.50f,0.35f);
    c[ImGuiCol_FrameBg]             = C(0.11f,0.15f,0.26f,1.f);
    c[ImGuiCol_FrameBgHovered]      = C(0.15f,0.21f,0.35f,1.f);
    c[ImGuiCol_FrameBgActive]       = C(0.18f,0.25f,0.42f,1.f);
    c[ImGuiCol_TitleBg]             = C(0.06f,0.09f,0.17f,1.f);
    c[ImGuiCol_TitleBgActive]       = C(0.08f,0.12f,0.22f,1.f);
    c[ImGuiCol_MenuBarBg]           = C(0.06f,0.09f,0.17f,1.f);
    c[ImGuiCol_ScrollbarBg]         = C(0.05f,0.07f,0.14f,1.f);
    c[ImGuiCol_ScrollbarGrab]       = C(0.20f,0.30f,0.50f,1.f);
    c[ImGuiCol_ScrollbarGrabHovered]= C(0.28f,0.40f,0.62f,1.f);
    c[ImGuiCol_CheckMark]           = C(0.36f,0.55f,0.84f,1.f);
    c[ImGuiCol_SliderGrab]          = C(0.36f,0.55f,0.84f,1.f);
    c[ImGuiCol_SliderGrabActive]    = C(0.48f,0.65f,0.88f,1.f);
    c[ImGuiCol_Button]              = C(0.18f,0.25f,0.42f,1.f);
    c[ImGuiCol_ButtonHovered]       = C(0.24f,0.34f,0.55f,1.f);
    c[ImGuiCol_ButtonActive]        = C(0.30f,0.42f,0.66f,1.f);
    c[ImGuiCol_Header]              = C(0.20f,0.30f,0.50f,0.7f);
    c[ImGuiCol_HeaderHovered]       = C(0.26f,0.38f,0.60f,0.9f);
    c[ImGuiCol_HeaderActive]        = C(0.32f,0.46f,0.72f,1.f);
    c[ImGuiCol_Separator]           = C(0.20f,0.30f,0.50f,0.35f);
    c[ImGuiCol_Text]                = C(0.91f,0.93f,0.97f,1.f);
    c[ImGuiCol_TextDisabled]        = C(0.55f,0.62f,0.75f,1.f);
    c[ImGuiCol_ModalWindowDimBg]    = C(0.0f,0.0f,0.0f,0.55f);
}

// --------- Logo texture ---------
static void ensureLogo() {
    if (g_logoTried) return;
    g_logoTried = true;
    if (RAVENXD_LOGO_PNG_SIZE > 4)
        LoadTextureFromMemory(RAVENXD_LOGO_PNG, RAVENXD_LOGO_PNG_SIZE, &g_logoTex);
}

// --------- Helpers ---------
static void drawTextColored(ImU32 col, const char* txt) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col));
    ImGui::TextUnformatted(txt);
    ImGui::PopStyleColor();
}

static bool sidebarButton(const char* label, bool active, float width, const char* icon) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 sz(width, 42);
    ImGui::InvisibleButton(label, sz);
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();

    ImU32 bg = active ? U32(45, 70, 120)
              : hovered ? U32(30, 44, 74)
              : 0;
    if (bg) {
        ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x+sz.x,p.y+sz.y), bg, 12.f);
    }
    if (active) {
        ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x+4,p.y+sz.y),
                                                  U32(91,141,214), 4.f);
    }
    ImU32 txtCol = active ? U32(232,238,248) : U32(170,185,210);
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(p.x+38, p.y+sz.y*0.5f-9),
        txtCol, label);
    if (icon && *icon) {
        ImGui::GetWindowDrawList()->AddText(ImVec2(p.x+16, p.y+sz.y*0.5f-9), txtCol, icon);
    }
    return clicked;
}

// --------- Sidebar ---------
static void DrawSidebar(Launcher& L, float width) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f,0.09f,0.17f,1.f));
    ImGui::BeginChild("##sidebar", ImVec2(width, 0), true);
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 18));

    // Logo
    ensureLogo();
    if (g_logoTex) {
        float sz = 56.f;
        float x = (width - sz) * 0.5f;
        ImGui::SetCursorPosX(x);
        ImGui::Image((ImTextureID)g_logoTex, ImVec2(sz,sz));
    } else {
        ImGui::SetCursorPosX(width*0.5f - 30);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.36f,0.55f,0.84f,1.f));
        ImGui::Text("RXD");
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0, 6));
    ImGui::SetCursorPosX(16);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.62f,0.75f,1.f));
    ImGui::Text("RavenXD");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 14));

    // Menu
    ImGui::SetCursorPosX(8);
    ImGui::BeginGroup();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
    if (sidebarButton("Home",     L.s().page == Page::Home,     width-16, "H")) L.s().page = Page::Home;
    if (sidebarButton("Versions", L.s().page == Page::Versions, width-16, "V")) L.s().page = Page::Versions;
    if (sidebarButton("Mods",     L.s().page == Page::Mods,     width-16, "M")) L.s().page = Page::Mods;
    if (sidebarButton("Settings", L.s().page == Page::Settings, width-16, "S")) L.s().page = Page::Settings;
    ImGui::PopStyleVar();
    ImGui::EndGroup();

    ImGui::EndChild();
}

// --------- Loading screen ---------
void UI::RenderLoadingScreen(Launcher& L) {
    auto& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(ImVec2(0,0), io.DisplaySize, COL_BG);

    float t = L.s().loadingTimer;
    float alpha = t < 0.6f ? t/0.6f : 1.f;
    float scale = 0.85f + 0.15f * (t > 1.0f ? 1.f : t);
    if (scale > 1.f) scale = 1.f;

    float cx = io.DisplaySize.x * 0.5f;
    float cy = io.DisplaySize.y * 0.5f - 30;

    float sz = 120.f * scale;
    if (g_logoTex) {
        dl->AddImage((ImTextureID)g_logoTex,
                     ImVec2(cx - sz*0.5f, cy - sz*0.5f),
                     ImVec2(cx + sz*0.5f, cy + sz*0.5f),
                     ImVec2(0,0), ImVec2(1,1),
                     IM_COL32(255,255,255,(int)(alpha*255)));
    } else {
        dl->AddText(ImVec2(cx-30, cy-10), IM_COL32(123,167,224,(int)(alpha*255)), "RavenXD");
    }

    const char* phase =
        t < 0.8f ? "Checking files..." :
        t < 1.6f ? "Loading launcher..." : "Ready";
    ImVec2 ts = ImGui::CalcTextSize(phase);
    dl->AddText(ImVec2(cx - ts.x*0.5f, cy + 90),
                IM_COL32(170,185,210,(int)(alpha*255)), phase);
}

// --------- HOME ---------
static void DrawHome(Launcher& L, float w, float h) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f,0.11f,0.20f,1.f));
    ImGui::BeginChild("##home", ImVec2(0,0), true);
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::SetCursorPosX(32);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.91f,0.93f,0.97f,1.f));
    ImGui::SetWindowFontScale(1.6f);
    ImGui::Text("RavenXD Launcher");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(32);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.62f,0.75f,1.f));
    ImGui::Text("Minecraft 1.8.9");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 24));

    // Profile card
    ImGui::SetCursorPosX(32);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f,0.15f,0.26f,1.f));
    ImGui::BeginChild("##profilecard", ImVec2(w - 64, 90), true);
    ImGui::PopStyleColor();
    ImGui::SetCursorPos(ImVec2(16, 16));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.62f,0.75f,1.f));
    ImGui::Text("Profile");
    ImGui::PopStyleColor();
    ImGui::SetCursorPos(ImVec2(16, 40));
    static char userBuf[64];
    strncpy_s(userBuf, Settings::I().d().username.c_str(), sizeof(userBuf)-1);
    ImGui::SetNextItemWidth(220);
    if (ImGui::InputText("##user", userBuf, sizeof(userBuf))) {
        Settings::I().d().username = userBuf;
        Settings::I().save();
    }
    ImGui::EndChild();

    ImGui::Dummy(ImVec2(0, 12));

    // Version selector
    ImGui::SetCursorPosX(32);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f,0.15f,0.26f,1.f));
    ImGui::BeginChild("##vercard", ImVec2(w - 64, 90), true);
    ImGui::PopStyleColor();
    ImGui::SetCursorPos(ImVec2(16, 16));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.62f,0.75f,1.f));
    ImGui::Text("Version");
    ImGui::PopStyleColor();
    ImGui::SetCursorPos(ImVec2(16, 40));
    const char* versions[] = { "Minecraft 1.8.9", "Forge 1.8.9", "Forge 1.8.9 + OptiFine" };
    ImGui::SetNextItemWidth(340);
    ImGui::Combo("##ver", &L.s().selectedVersion, versions, 3);
    ImGui::EndChild();

    ImGui::Dummy(ImVec2(0, 24));

    // LAUNCH button
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float bw = 380, bh = 68;
    ImGui::SetCursorPosX((avail.x - bw) * 0.5f + 16);
    bool busy = L.s().taskState == TaskState::Running;

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.36f,0.55f,0.84f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f,0.65f,0.88f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f,0.45f,0.75f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.f);

    // Glow
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(p.x-6, p.y-6), ImVec2(p.x+bw+6, p.y+bh+6),
        U32(91,141,214,50), 24.f);

    if (!busy) {
        ImGui::SetWindowFontScale(1.5f);
        if (ImGui::Button("   >  LAUNCH", ImVec2(bw, bh))) {
            L.onLaunchClicked();
        }
        ImGui::SetWindowFontScale(1.0f);
    } else {
        ImGui::BeginDisabled();
        ImGui::SetWindowFontScale(1.5f);
        ImGui::Button("   ...  WORKING", ImVec2(bw, bh));
        ImGui::SetWindowFontScale(1.0f);
        ImGui::EndDisabled();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::Dummy(ImVec2(0, 20));

    // Progress bar
    ImGui::SetCursorPosX(32);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.36f,0.55f,0.84f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0.11f,0.15f,0.26f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f);
    float pct = L.s().progress;
    char overlay[64];
    if (L.s().speedMBps > 0.f)
        snprintf(overlay, sizeof(overlay), "%d%%   %.2f MB/s", (int)(pct*100.f), L.s().speedMBps);
    else
        snprintf(overlay, sizeof(overlay), "%d%%", (int)(pct*100.f));
    ImGui::ProgressBar(pct, ImVec2(w - 64, 20), overlay);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    ImGui::Dummy(ImVec2(0, 8));
    ImGui::SetCursorPosX(32);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
    ImGui::Text("%s", L.s().statusText.c_str());
    ImGui::PopStyleColor();

    if (L.s().taskState == TaskState::Failed && !L.s().lastError.empty()) {
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::SetCursorPosX(32);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f,0.45f,0.45f,1.f));
        ImGui::Text("Error: %s", L.s().lastError.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
}

// --------- VERSIONS ---------
static void DrawVersions(Launcher& L, float w, float h) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f,0.11f,0.20f,1.f));
    ImGui::BeginChild("##versions", ImVec2(0,0), true);
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::SetCursorPosX(32);
    ImGui::SetWindowFontScale(1.4f);
    ImGui::Text("Versions");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0, 18));

    const char* names[]   = { "Minecraft 1.8.9", "Forge 1.8.9", "Forge 1.8.9 + OptiFine" };
    const char* descs[]   = {
        "Vanilla Minecraft 1.8.9 - official Mojang client.",
        "Forge 1.8.9 - mod loader (required for mods).",
        "Forge 1.8.9 + OptiFine - recommended for performance & shaders."
    };

    for (int i = 0; i < 3; ++i) {
        ImGui::SetCursorPosX(32);
        bool sel = (L.s().selectedVersion == i);
        ImGui::PushStyleColor(ImGuiCol_ChildBg,
            sel ? ImVec4(0.18f,0.28f,0.48f,1.f) : ImVec4(0.11f,0.15f,0.26f,1.f));
        ImGui::BeginChild((std::string("##vcard")+std::to_string(i)).c_str(),
                          ImVec2(w - 64, 90), true);
        ImGui::PopStyleColor();
        ImGui::SetCursorPos(ImVec2(18, 14));
        ImGui::SetWindowFontScale(1.1f);
        ImGui::Text("%s", names[i]);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::SetCursorPos(ImVec2(18, 42));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
        ImGui::TextWrapped("%s", descs[i]);
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(w - 160, 30));
        if (sel) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.36f,0.55f,0.84f,1.f));
            ImGui::Button("Selected", ImVec2(110, 32));
            ImGui::PopStyleColor();
        } else {
            if (ImGui::Button("Select", ImVec2(110, 32))) {
                L.s().selectedVersion = i;
                Settings::I().d().version = names[i];
                Settings::I().save();
            }
        }
        ImGui::EndChild();
        ImGui::Dummy(ImVec2(0, 10));
    }

    ImGui::EndChild();
}

// --------- MODS ---------
static void DrawMods(Launcher& L, float w, float h) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f,0.11f,0.20f,1.f));
    ImGui::BeginChild("##mods", ImVec2(0,0), true);
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::SetCursorPosX(32);
    ImGui::SetWindowFontScale(1.4f);
    ImGui::Text("Mods");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::SameLine(w - 180);
    if (ImGui::Button("Update All", ImVec2(120, 34))) {
        std::string dir = Settings::I().d().minecraftDir;
        L.mods().updateAll(dir, [&L](const std::string& s, float p){
            // set status (threadless here, but for demo just update state)
            L.s().statusText = s;
            L.s().progress = p;
        });
    }

    ImGui::Dummy(ImVec2(0, 18));

    auto& mods = L.mods().mods();
    for (size_t i = 0; i < mods.size(); ++i) {
        auto& m = mods[i];
        ImGui::SetCursorPosX(32);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f,0.15f,0.26f,1.f));
        ImGui::BeginChild((std::string("##mcard")+std::to_string(i)).c_str(),
                          ImVec2(w - 64, 100), true);
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(18, 12));
        ImGui::SetWindowFontScale(1.1f);
        ImGui::Text("%s", m.name.c_str());
        ImGui::SetWindowFontScale(1.0f);

        ImGui::SetCursorPos(ImVec2(18, 40));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
        ImGui::Text("Version %s", m.version.c_str());
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(18, 62));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f,0.55f,0.72f,1.f));
        ImGui::Text("%s  %s", m.source.c_str(), m.installed ? "[installed]" : "[not installed]");
        ImGui::PopStyleColor();

        // Toggle
        ImGui::SetCursorPos(ImVec2(w - 260, 30));
        ImGui::Text(m.enabled ? "ON" : "OFF");
        ImGui::SameLine();
        if (ImGui::Button(m.enabled ? "Disable" : "Enable", ImVec2(80, 30))) {
            L.mods().setEnabled(i, !m.enabled, Settings::I().d().minecraftDir);
        }
        ImGui::SameLine();
        if (ImGui::Button("Update", ImVec2(80, 30))) {
            L.mods().updateAll(Settings::I().d().minecraftDir, nullptr);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete", ImVec2(80, 30))) {
            L.mods().remove(i, Settings::I().d().minecraftDir);
        }

        ImGui::EndChild();
        ImGui::Dummy(ImVec2(0, 10));
    }

    ImGui::EndChild();
}

// --------- SETTINGS ---------
static void DrawSettings(Launcher& L, float w, float h) {
    auto& d = Settings::I().d();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f,0.11f,0.20f,1.f));
    ImGui::BeginChild("##settings", ImVec2(0,0), true);
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::SetCursorPosX(32);
    ImGui::SetWindowFontScale(1.4f);
    ImGui::Text("Settings");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Dummy(ImVec2(0, 18));

    auto cardStart = [&](const char* id, float height){
        ImGui::SetCursorPosX(32);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f,0.15f,0.26f,1.f));
        ImGui::BeginChild(id, ImVec2(w - 64, height), true);
        ImGui::PopStyleColor();
    };

    // Minecraft dir
    cardStart("##c1", 80);
    ImGui::SetCursorPos(ImVec2(16, 12));
    ImGui::Text("Minecraft Directory");
    ImGui::SetCursorPos(ImVec2(16, 40));
    static char dirBuf[512];
    strncpy_s(dirBuf, d.minecraftDir.c_str(), sizeof(dirBuf)-1);
    ImGui::SetNextItemWidth(w - 140);
    if (ImGui::InputText("##dir", dirBuf, sizeof(dirBuf))) {
        d.minecraftDir = dirBuf;
        Settings::I().save();
    }
    ImGui::EndChild();
    ImGui::Dummy(ImVec2(0, 10));

    // Java path
    cardStart("##c2", 80);
    ImGui::SetCursorPos(ImVec2(16, 12));
    ImGui::Text("Java Path");
    ImGui::SetCursorPos(ImVec2(16, 40));
    static char javaBuf[512];
    strncpy_s(javaBuf, d.javaPath.c_str(), sizeof(javaBuf)-1);
    ImGui::SetNextItemWidth(w - 260);
    if (ImGui::InputText("##java", javaBuf, sizeof(javaBuf))) {
        d.javaPath = javaBuf;
        Settings::I().save();
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Detect", ImVec2(120, 0))) {
        d.javaPath = Minecraft::FindJava();
        Settings::I().save();
        strncpy_s(javaBuf, d.javaPath.c_str(), sizeof(javaBuf)-1);
    }
    ImGui::EndChild();
    ImGui::Dummy(ImVec2(0, 10));

    // RAM + Window
    cardStart("##c3", 130);
    ImGui::SetCursorPos(ImVec2(16, 12));
    ImGui::Text("RAM (MB)");
    ImGui::SetCursorPos(ImVec2(16, 40));
    ImGui::SetNextItemWidth(220);
    if (ImGui::SliderInt("##ram", &d.ramMB, 512, 16384)) Settings::I().save();

    ImGui::SetCursorPos(ImVec2(16, 76));
    ImGui::Text("Window Size");
    ImGui::SetCursorPos(ImVec2(120, 76));
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("##ww", &d.windowWidth, 0, 0)) Settings::I().save();
    ImGui::SameLine(); ImGui::Text("x"); ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("##wh", &d.windowHeight, 0, 0)) Settings::I().save();
    ImGui::EndChild();
    ImGui::Dummy(ImVec2(0, 10));

    // Toggles
    cardStart("##c4", 110);
    ImGui::SetCursorPos(ImVec2(16, 14));
    if (ImGui::Checkbox("Close launcher after launch", &d.closeAfterLaunch))
        Settings::I().save();
    ImGui::SetCursorPos(ImVec2(16, 50));
    if (ImGui::Checkbox("Debug Mode", &d.debugMode))
        Settings::I().save();
    ImGui::SetCursorPos(ImVec2(16, 82));
    if (ImGui::Button("Open RavenXD Folder", ImVec2(200, 24))) {
        std::string p = Settings::I().appDataDir();
        ShellExecuteA(nullptr, "open", p.c_str(), nullptr, nullptr, SW_SHOW);
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

// --------- Java popup ---------
static void DrawJavaPopup(Launcher& L) {
    if (L.s().showJavaPopup) ImGui::OpenPopup("Java");
    ImGui::SetNextWindowSize(ImVec2(400, 0));
    if (ImGui::BeginPopupModal("Java", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.95f,0.55f,0.55f,1.f), "Java not found");
        ImGui::Separator();
        ImGui::TextWrapped("RavenXD couldn't locate a Java 8 installation. "
                           "Please pick your javaw.exe manually, or install Java 8 (Adoptium Temurin 8).");
        ImGui::Dummy(ImVec2(0, 8));
        if (ImGui::Button("Install / Select Java", ImVec2(180, 34))) {
            // Open file picker (simple approach: use OpenFileName)
            char file[MAX_PATH] = {};
            OPENFILENAMEA ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner   = GetActiveWindow();
            ofn.lpstrFilter = "Java executable\0javaw.exe;java.exe\0All Files\0*.*\0";
            ofn.lpstrFile   = file;
            ofn.nMaxFile    = MAX_PATH;
            ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileNameA(&ofn)) {
                Settings::I().d().javaPath = file;
                Settings::I().save();
                L.s().javaOk = true;
                L.s().showJavaPopup = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Get Java 8", ImVec2(120, 34))) {
            ShellExecuteA(nullptr, "open",
                "https://adoptium.net/temurin/releases/?version=8",
                nullptr, nullptr, SW_SHOW);
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(80, 34))) {
            L.s().showJavaPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// --------- Root ---------
void UI::Render(Launcher& L, float dt) {
    L.tick(dt);

    if (L.s().showLoadingScreen) {
        RenderLoadingScreen(L);
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    // Background
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(0,0), io.DisplaySize, COL_BG);

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##root", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground);

    float sidebarW = 200.f;
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::BeginChild("##container", ImVec2(0,0), false);

    // Sidebar
    ImGui::BeginChild("##sbwrap", ImVec2(sidebarW, 0), false);
    DrawSidebar(L, sidebarW);
    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // Content
    ImGui::BeginChild("##content", ImVec2(0, 0), false);
    ImVec2 avail = ImGui::GetContentRegionAvail();
    switch (L.s().page) {
        case Page::Home:     DrawHome(L, avail.x, avail.y); break;
        case Page::Versions: DrawVersions(L, avail.x, avail.y); break;
        case Page::Mods:     DrawMods(L, avail.x, avail.y); break;
        case Page::Settings: DrawSettings(L, avail.x, avail.y); break;
    }
    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::End();

    DrawJavaPopup(L);
}
