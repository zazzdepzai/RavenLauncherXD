#include "UI.h"
#include "../DiscordRPC.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "embedded_assets.h"
#include <d3d11.h>
#include <windows.h>
#include <string>
#include <cstdio>
#include <cmath>

extern ID3D11Device*           g_pd3dDevice;
extern ID3D11DeviceContext*    g_pd3dDeviceContext;
extern ImFont*                 g_fontRegular;
extern ImFont*                 g_fontBold;
extern ImFont*                 g_fontBig;
extern void LoadTextureFromMemory(const unsigned char* data, size_t len,
                                  ID3D11ShaderResourceView** out);

ID3D11ShaderResourceView*      g_logoTex = nullptr;
static bool                    g_logoTried = false;

// ======================= PALETTE =======================
static ImU32 U32(int r, int g, int b, int a=255){ return IM_COL32(r,g,b,a); }

static const ImU32 COL_BG          = U32(10, 14, 26);
static const ImU32 COL_BG2         = U32(18, 24, 44);
static const ImU32 COL_SIDEBAR     = U32(14, 19, 34);
static const ImU32 COL_TOPBAR      = U32(14, 19, 34);
static const ImU32 COL_CARD        = U32(24, 32, 54);
static const ImU32 COL_CARD_HOV    = U32(32, 44, 72);
static const ImU32 COL_TEXT        = U32(232, 238, 250);
static const ImU32 COL_TEXT_DIM    = U32(150, 165, 195);
static const ImU32 COL_TEXT_DIMMER = U32(100, 115, 145);
static const ImU32 COL_ACCENT      = U32(80, 140, 230);
static const ImU32 COL_ACCENT_HI   = U32(110, 165, 245);
static const ImU32 COL_GREEN       = U32(80, 200, 120);

// ======================= STATE =======================
static int g_detailIdx = -1;
static char g_userBuf[64] = "Player";
static bool g_userInit = false;

// ======================= STYLE =======================
void UI::ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding      = 14.f;
    s.ChildRounding       = 14.f;
    s.FrameRounding       = 10.f;
    s.PopupRounding       = 14.f;
    s.ScrollbarRounding   = 10.f;
    s.GrabRounding        = 10.f;
    s.TabRounding         = 10.f;
    s.WindowBorderSize    = 0.f;
    s.FrameBorderSize     = 0.f;
    s.WindowPadding       = ImVec2(0,0);
    s.FramePadding        = ImVec2(12, 8);
    s.ItemSpacing         = ImVec2(10, 10);
    s.ScrollbarSize       = 8.f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]         = ImVec4(0.04f,0.055f,0.10f,1.f);
    c[ImGuiCol_ChildBg]          = ImVec4(0.f,0.f,0.f,0.f);
    c[ImGuiCol_PopupBg]          = ImVec4(0.08f,0.11f,0.20f,1.f);
    c[ImGuiCol_Border]           = ImVec4(0.20f,0.30f,0.50f,0.25f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.10f,0.14f,0.24f,1.f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.14f,0.20f,0.34f,1.f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.18f,0.25f,0.42f,1.f);
    c[ImGuiCol_Button]           = ImVec4(0.18f,0.25f,0.42f,1.f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.24f,0.34f,0.55f,1.f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.30f,0.42f,0.66f,1.f);
    c[ImGuiCol_Header]           = ImVec4(0.20f,0.30f,0.50f,0.7f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.26f,0.38f,0.60f,0.9f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.32f,0.46f,0.72f,1.f);
    c[ImGuiCol_Separator]        = ImVec4(0.20f,0.30f,0.50f,0.35f);
    c[ImGuiCol_Text]             = ImVec4(0.91f,0.93f,0.97f,1.f);
    c[ImGuiCol_TextDisabled]     = ImVec4(0.55f,0.62f,0.75f,1.f);
    c[ImGuiCol_CheckMark]        = ImVec4(0.36f,0.55f,0.84f,1.f);
    c[ImGuiCol_SliderGrab]       = ImVec4(0.36f,0.55f,0.84f,1.f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.48f,0.65f,0.88f,1.f);
    c[ImGuiCol_ScrollbarBg]      = ImVec4(0.f,0.f,0.f,0.f);
    c[ImGuiCol_ScrollbarGrab]    = ImVec4(0.24f,0.34f,0.55f,1.f);
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f,0.0f,0.0f,0.65f);
}

// ======================= LOGO =======================
static void ensureLogo() {
    if (g_logoTried) return;
    g_logoTried = true;
    if (RAVENXD_LOGO_PNG_SIZE > 4)
        LoadTextureFromMemory(RAVENXD_LOGO_PNG, RAVENXD_LOGO_PNG_SIZE, &g_logoTex);
}

// ======================= GRADIENT HELPER =======================
// Vẽ gradient dọc từ màu A (trên) xuống màu B (dưới), bo góc r
static void DrawGradientV(ImDrawList* dl, ImVec2 p1, ImVec2 p2,
                          ImU32 colTop, ImU32 colBot, float rounding)
{
    int steps = 24;
    float h = p2.y - p1.y;
    for (int i = 0; i < steps; ++i) {
        float t0 = (float)i / steps;
        float t1 = (float)(i+1) / steps;
        ImU32 c0 = ImGui::GetColorU32(ImLerp(
            ImGui::ColorConvertU32ToFloat4(colTop),
            ImGui::ColorConvertU32ToFloat4(colBot), t0));
        ImU32 c1 = ImGui::GetColorU32(ImLerp(
            ImGui::ColorConvertU32ToFloat4(colTop),
            ImGui::ColorConvertU32ToFloat4(colBot), t1));
        (void)c1;
        dl->AddRectFilled(
            ImVec2(p1.x, p1.y + h*t0),
            ImVec2(p2.x, p1.y + h*t1),
            c0, 0.f);
    }
    // Bo góc 4 cạnh = vẽ 4 vòng cung nhỏ — dùng AddRectFilled với rounding ở trên cùng/ dưới cùng
    if (rounding > 0.f) {
        dl->AddRectFilled(p1, ImVec2(p2.x, p1.y + rounding), colTop, rounding, ImDrawFlags_RoundCornersTop);
        dl->AddRectFilled(ImVec2(p1.x, p2.y - rounding), p2, colBot, rounding, ImDrawFlags_RoundCornersBottom);
    }
}

// ======================= ICONS =======================
static void IconHome(ImDrawList* dl, ImVec2 c, float sz, ImU32 col) {
    dl->AddTriangleFilled(
        ImVec2(c.x, c.y - sz*0.55f),
        ImVec2(c.x - sz*0.6f, c.y + sz*0.05f),
        ImVec2(c.x + sz*0.6f, c.y + sz*0.05f), col);
    dl->AddRectFilled(
        ImVec2(c.x - sz*0.4f, c.y + sz*0.05f),
        ImVec2(c.x + sz*0.4f, c.y + sz*0.6f),
        col, 2.f);
}

static void IconVersions(ImDrawList* dl, ImVec2 c, float sz, ImU32 col) {
    dl->AddRect(ImVec2(c.x - sz*0.5f, c.y - sz*0.5f),
                ImVec2(c.x + sz*0.5f, c.y + sz*0.5f),
                col, 3.f, 0, 2.f);
    dl->AddLine(ImVec2(c.x - sz*0.3f, c.y - sz*0.2f),
                ImVec2(c.x + sz*0.3f, c.y - sz*0.2f), col, 2.f);
    dl->AddLine(ImVec2(c.x - sz*0.3f, c.y + sz*0.1f),
                ImVec2(c.x + sz*0.3f, c.y + sz*0.1f), col, 2.f);
    dl->AddLine(ImVec2(c.x - sz*0.3f, c.y + sz*0.3f),
                ImVec2(c.x + sz*0.1f, c.y + sz*0.3f), col, 2.f);
}

static void IconMods(ImDrawList* dl, ImVec2 c, float sz, ImU32 col) {
    dl->AddCircleFilled(ImVec2(c.x, c.y), sz*0.5f, col);
    dl->AddCircleFilled(ImVec2(c.x, c.y), sz*0.22f, COL_SIDEBAR);
    for (int i = 0; i < 6; ++i) {
        float a = i * IM_PI / 3.f;
        ImVec2 p1(c.x + cosf(a)*sz*0.5f,  c.y + sinf(a)*sz*0.5f);
        ImVec2 p2(c.x + cosf(a)*sz*0.7f,  c.y + sinf(a)*sz*0.7f);
        dl->AddLine(p1, p2, col, 2.5f);
    }
}

static void IconSettings(ImDrawList* dl, ImVec2 c, float sz, ImU32 col) {
    dl->AddCircleFilled(c, sz*0.5f, col);
    dl->AddCircleFilled(c, sz*0.22f, COL_SIDEBAR);
    for (int i = 0; i < 8; ++i) {
        float a = i * IM_PI / 4.f;
        ImVec2 p1(c.x + cosf(a)*sz*0.5f, c.y + sinf(a)*sz*0.5f);
        ImVec2 p2(c.x + cosf(a)*sz*0.72f, c.y + sinf(a)*sz*0.72f);
        dl->AddLine(p1, p2, col, 2.5f);
    }
}

static void IconExit(ImDrawList* dl, ImVec2 c, float sz, ImU32 col) {
    dl->AddRect(ImVec2(c.x - sz*0.5f, c.y - sz*0.5f),
                ImVec2(c.x + sz*0.4f, c.y + sz*0.5f),
                col, 2.f, 0, 2.f);
    dl->AddLine(ImVec2(c.x - sz*0.1f, c.y),
                ImVec2(c.x + sz*0.65f, c.y), col, 2.f);
    dl->AddTriangleFilled(
        ImVec2(c.x + sz*0.55f, c.y - sz*0.22f),
        ImVec2(c.x + sz*0.55f, c.y + sz*0.22f),
        ImVec2(c.x + sz*0.85f, c.y), col);
}

// ======================= SIDEBAR =======================
typedef void (*IconFn)(ImDrawList*, ImVec2, float, ImU32);

static bool SidebarButton(ImDrawList* dl, ImVec2 pos, float size, bool active,
                          IconFn icon, const char* id, const char* tooltip)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(id, ImVec2(size, size));
    bool hov = ImGui::IsItemHovered();
    bool clk = ImGui::IsItemClicked();

    ImVec2 c(pos.x + size*0.5f, pos.y + size*0.5f);

    if (active) {
        dl->AddRectFilled(pos, ImVec2(pos.x+size, pos.y+size), U32(40,70,120,220), 12.f);
        // vạch accent bên trái
        dl->AddRectFilled(pos, ImVec2(pos.x+3, pos.y+size), COL_ACCENT, 2.f);
    } else if (hov) {
        dl->AddRectFilled(pos, ImVec2(pos.x+size, pos.y+size), U32(30,42,70,180), 12.f);
    }

    ImU32 col = active ? COL_ACCENT_HI : (hov ? COL_TEXT : COL_TEXT_DIM);
    icon(dl, c, size*0.42f, col);

    if (hov && tooltip && *tooltip) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip);
        ImGui::EndTooltip();
    }
    return clk;
}

static void DrawSidebar(Launcher& L, ImDrawList* dl, ImVec2 pos, float w, float h) {
    DrawGradientV(dl, pos, ImVec2(pos.x+w, pos.y+h),
                  COL_SIDEBAR, U32(10,15,28), 14.f);

    float btn = 48.f;
    float x = pos.x + (w - btn) * 0.5f;
    float y = pos.y + 20.f;

    if (SidebarButton(dl, ImVec2(x,y), btn, L.s().page == Page::Home,
                      IconHome, "##home", "Home"))
    {
        L.s().page = Page::Home; g_detailIdx = -1;
    }
    y += btn + 10;

    if (SidebarButton(dl, ImVec2(x,y), btn, L.s().page == Page::Versions,
                      IconVersions, "##vers", "Versions"))
    {
        L.s().page = Page::Versions; g_detailIdx = -1;
    }
    y += btn + 10;

    if (SidebarButton(dl, ImVec2(x,y), btn, L.s().page == Page::Mods,
                      IconMods, "##mods", "Mods"))
    {
        L.s().page = Page::Mods; g_detailIdx = -1;
    }
    y += btn + 10;

    if (SidebarButton(dl, ImVec2(x,y), btn, L.s().page == Page::Settings,
                      IconSettings, "##set", "Settings"))
    {
        L.s().page = Page::Settings; g_detailIdx = -1;
    }

    // Discord status
    {
        float dy = pos.y + h - btn - 20 - 40;
        ImVec2 dc(x + btn*0.5f, dy + btn*0.5f);
        bool ok = DiscordRPC::I().isReady();
        if (ok) {
            dl->AddCircleFilled(dc, btn*0.28f, U32(80,200,120,60));
            dl->AddCircleFilled(dc, btn*0.16f, COL_GREEN);
        } else {
            dl->AddCircleFilled(dc, btn*0.16f, U32(110,110,125));
        }
        ImGui::SetCursorScreenPos(ImVec2(x, dy));
        ImGui::InvisibleButton("##dc", ImVec2(btn,btn));
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextColored(ok ? ImVec4(0.31f,0.78f,0.47f,1.f)
                                  : ImVec4(0.6f,0.6f,0.65f,1.f),
                               ok ? "Discord: Connected" : "Discord: Not running");
            ImGui::EndTooltip();
        }
    }

    // Exit
    float exY = pos.y + h - btn - 20;
    if (SidebarButton(dl, ImVec2(x, exY), btn, false, IconExit, "##exit", "Exit"))
        PostQuitMessage(0);
}

// ======================= TOP BAR =======================
static void DrawTopBar(ImDrawList* dl, ImVec2 pos, float w, float h, const char* title) {
    DrawGradientV(dl, pos, ImVec2(pos.x+w, pos.y+h),
                  COL_TOPBAR, U32(10,15,28), 14.f);

    ensureLogo();
    float ls = 28.f;
    ImVec2 lp(pos.x + 18, pos.y + (h-ls)*0.5f);
    if (g_logoTex)
        dl->AddImage((ImTextureID)g_logoTex, lp, ImVec2(lp.x+ls, lp.y+ls));

    ImGui::PushFont(g_fontBold);
    ImVec2 tp(lp.x + ls + 12, pos.y + (h-18)*0.5f);
    dl->AddText(tp, COL_TEXT, title);
    ImGui::PopFont();

    float bw = 46.f, bh = h;

    // Minimize
    ImVec2 mp(pos.x + w - bw*2, pos.y);
    ImGui::SetCursorScreenPos(mp);
    ImGui::InvisibleButton("##min", ImVec2(bw,bh));
    bool mh = ImGui::IsItemHovered();
    if (mh) dl->AddRectFilled(mp, ImVec2(mp.x+bw, mp.y+bh), U32(50,65,95,180));
    dl->AddLine(ImVec2(mp.x+bw*0.35f, mp.y+bh*0.5f),
                ImVec2(mp.x+bw*0.65f, mp.y+bh*0.5f),
                mh ? COL_TEXT : COL_TEXT_DIM, 1.6f);
    if (ImGui::IsItemClicked()) ShowWindow(GetActiveWindow(), SW_MINIMIZE);

    // Close
    ImVec2 cp(pos.x + w - bw, pos.y);
    ImGui::SetCursorScreenPos(cp);
    ImGui::InvisibleButton("##close", ImVec2(bw,bh));
    bool ch = ImGui::IsItemHovered();
    if (ch) dl->AddRectFilled(cp, ImVec2(cp.x+bw, cp.y+bh), U32(220,60,60,200));
    ImU32 xc = ch ? U32(255,255,255) : COL_TEXT_DIM;
    dl->AddLine(ImVec2(cp.x+bw*0.35f, cp.y+bh*0.35f),
                ImVec2(cp.x+bw*0.65f, cp.y+bh*0.65f), xc, 1.6f);
    dl->AddLine(ImVec2(cp.x+bw*0.65f, cp.y+bh*0.35f),
                ImVec2(cp.x+bw*0.35f, cp.y+bh*0.65f), xc, 1.6f);
    if (ImGui::IsItemClicked()) PostQuitMessage(0);
}

// ======================= HOME PAGE =======================
static void DrawHome(Launcher& L, ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    DrawGradientV(dl, pos, ImVec2(pos.x+size.x, pos.y+size.y), COL_BG, COL_BG2, 14.f);

    if (!g_userInit) {
        strncpy_s(g_userBuf, Settings::I().d().username.c_str(), sizeof(g_userBuf)-1);
        g_userInit = true;
    }

    float pad = 32.f;

    // Title
    ImGui::PushFont(g_fontBig);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.91f,0.93f,0.97f,1.f));
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad));
    ImGui::Text("RavenXD");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 40));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.62f,0.75f,1.f));
    ImGui::Text("Minecraft 1.8.9  •  Forge  •  OptiFine");
    ImGui::PopStyleColor();

    // Card: Profile
    float cy = pos.y + pad + 90;
    ImVec2 card1(pos.x + pad, cy);
    ImVec2 card1End(pos.x + size.x - pad, cy + 72);
    dl->AddRectFilled(card1, card1End, COL_CARD, 12.f);
    dl->AddRect(card1, card1End, U32(60,80,120,80), 12.f, 0, 1.f);

    ImGui::SetCursorScreenPos(ImVec2(card1.x + 16, card1.y + 12));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
    ImGui::Text("Profile");
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(card1.x + 16, card1.y + 34));
    ImGui::SetNextItemWidth(280);
    if (ImGui::InputText("##user", g_userBuf, sizeof(g_userBuf))) {
        Settings::I().d().username = g_userBuf;
        Settings::I().save();
    }

    // Card: Version
    cy += 84;
    ImVec2 card2(pos.x + pad, cy);
    ImVec2 card2End(pos.x + size.x - pad, cy + 72);
    dl->AddRectFilled(card2, card2End, COL_CARD, 12.f);
    dl->AddRect(card2, card2End, U32(60,80,120,80), 12.f, 0, 1.f);

    ImGui::SetCursorScreenPos(ImVec2(card2.x + 16, card2.y + 12));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
    ImGui::Text("Version");
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(card2.x + 16, card2.y + 34));
    const char* versions[] = { "Minecraft 1.8.9", "Forge 1.8.9", "Forge 1.8.9 + OptiFine" };
    ImGui::SetNextItemWidth(380);
    ImGui::Combo("##ver", &L.s().selectedVersion, versions, 3);

    // LAUNCH button
    cy += 100;
    float bw = 340, bh = 60;
    ImVec2 bp(pos.x + pad, cy);

    ImGui::SetCursorScreenPos(bp);
    bool busy = L.s().taskState == TaskState::Running;

    // Glow
    dl->AddRectFilled(ImVec2(bp.x-8, bp.y-8),
                     ImVec2(bp.x+bw+8, bp.y+bh+8),
                     U32(80,140,230,50), 22.f);

    ImGui::PushFont(g_fontBold);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.31f,0.55f,0.90f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.43f,0.65f,0.95f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.25f,0.45f,0.80f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 18.f);

    if (!busy) {
        if (ImGui::Button("    LAUNCH", ImVec2(bw, bh))) L.onLaunchClicked();
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("    WORKING...", ImVec2(bw, bh));
        ImGui::EndDisabled();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::PopFont();

    // Progress bar
    cy += 84;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, cy));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.36f,0.55f,0.84f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0.11f,0.15f,0.26f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
    float pct = L.s().progress;
    char ov[64];
    if (L.s().speedMBps > 0.f)
        snprintf(ov, sizeof(ov), "%d%%   %.2f MB/s", (int)(pct*100.f), L.s().speedMBps);
    else
        snprintf(ov, sizeof(ov), "%d%%", (int)(pct*100.f));
    ImGui::ProgressBar(pct, ImVec2(size.x - pad*2, 22), ov);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    cy += 34;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, cy));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
    ImGui::Text("%s", L.s().statusText.c_str());
    ImGui::PopStyleColor();

    if (L.s().taskState == TaskState::Failed && !L.s().lastError.empty()) {
        cy += 22;
        ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, cy));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f,0.45f,0.45f,1.f));
        ImGui::TextWrapped("Error: %s", L.s().lastError.c_str());
        ImGui::PopStyleColor();
    }
}

// ======================= VERSIONS =======================
struct VerItem {
    const char* title;
    const char* sub;
    const char* tag;
    bool free;
    int mapSel;
    ImU32 c1, c2;
};

static VerItem g_versions[4] = {
    { "1.16.5",        "Client", "Client", false, 0, U32(120,40,60),  U32(180,60,30) },
    { "ALPHA 1.16.5",  "Client", "Client", false, 1, U32(20,60,60),   U32(40,150,120) },
    { "LEGACY 1.12.2", "Client", "Free",   true,  2, U32(100,140,200),U32(60,100,160) },
    { "1.21.11",       "Client", "Client", false, 1, U32(40,100,50),  U32(100,160,80) },
};

static bool VersionCard(ImVec2 pos, ImVec2 size, const VerItem& v, bool selected) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(v.title, size);
    bool hov = ImGui::IsItemHovered();
    bool clk = ImGui::IsItemClicked();

    DrawGradientV(dl, pos, ImVec2(pos.x+size.x, pos.y+size.y), v.c1, v.c2, 12.f);
    // Tối hơn nếu không hover
    dl->AddRectFilled(pos, ImVec2(pos.x+size.x, pos.y+size.y),
                     U32(0,0,0, hov ? 40 : 100), 12.f);

    if (selected)
        dl->AddRect(pos, ImVec2(pos.x+size.x, pos.y+size.y), COL_ACCENT, 12.f, 0, 3.f);
    else if (hov)
        dl->AddRect(pos, ImVec2(pos.x+size.x, pos.y+size.y), U32(140,180,240,220), 12.f, 0, 2.f);

    // Tag
    const char* tag = v.tag;
    ImVec2 ts = ImGui::CalcTextSize(tag);
    ImVec2 tpos(pos.x + size.x - ts.x - 24, pos.y + 10);
    ImU32 tbg = v.free ? U32(70,170,90) : U32(30,40,66,230);
    dl->AddRectFilled(tpos, ImVec2(tpos.x + ts.x + 20, tpos.y + ts.y + 10), tbg, 6.f);
    dl->AddText(ImVec2(tpos.x + 10, tpos.y + 5), U32(255,255,255), tag);

    // Title dưới
    ImVec2 tt = ImGui::CalcTextSize(v.title);
    ImVec2 tp(pos.x + 14, pos.y + size.y - tt.y - 18);
    dl->AddText(ImVec2(tp.x+1, tp.y+1), U32(0,0,0,180), v.title);
    dl->AddText(tp, U32(255,255,255), v.title);

    // Arrow
    ImVec2 ac(pos.x + size.x - 22, pos.y + size.y - 22);
    ImU32 ac2 = hov ? U32(255,255,255) : U32(255,255,255,200);
    if (hov) dl->AddCircleFilled(ac, 14.f, U32(91,141,214,120));
    float al = 6.f;
    dl->AddLine(ImVec2(ac.x-al, ac.y), ImVec2(ac.x+al, ac.y), ac2, 2.f);
    dl->AddLine(ImVec2(ac.x+al, ac.y), ImVec2(ac.x+al*0.3f, ac.y-al*0.6f), ac2, 2.f);
    dl->AddLine(ImVec2(ac.x+al, ac.y), ImVec2(ac.x+al*0.3f, ac.y+al*0.6f), ac2, 2.f);

    return clk;
}

static void DrawVersionsPage(Launcher& L, ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    DrawGradientV(dl, pos, ImVec2(pos.x+size.x, pos.y+size.y), COL_BG, COL_BG2, 14.f);

    float pad = 24.f;
    float gap = 16.f;
    int cols = 3;
    float cardW = (size.x - pad*2 - gap*(cols-1)) / cols;
    float cardH = 160.f;

    for (int i = 0; i < 4; ++i) {
        int row = i / cols, col = i % cols;
        ImVec2 cp(pos.x + pad + col*(cardW+gap),
                  pos.y + pad + row*(cardH+gap));
        if (cp.x + cardW > pos.x + size.x - pad) continue;

        bool sel = (L.s().selectedVersion == g_versions[i].mapSel);
        if (VersionCard(cp, ImVec2(cardW, cardH), g_versions[i], sel)) {
            // Direct launch
            L.s().selectedVersion = g_versions[i].mapSel;
            Settings::I().d().version = g_versions[i].title;
            Settings::I().save();
            L.onLaunchClicked();
        }
    }
}

// ======================= MODS =======================
static void DrawMods(Launcher& L, ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    DrawGradientV(dl, pos, ImVec2(pos.x+size.x, pos.y+size.y), COL_BG, COL_BG2, 14.f);

    float pad = 24.f;

    ImGui::PushFont(g_fontBold);
    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad, pos.y+pad));
    ImGui::Text("Mods");
    ImGui::PopFont();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x - 150, pos.y + pad + 4));
    if (ImGui::Button("Update All", ImVec2(120, 32))) {
        L.mods().updateAll(Settings::I().d().minecraftDir, nullptr);
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 50));
    ImGui::BeginChild("##modscroll", ImVec2(size.x - pad*2, size.y - pad*2 - 50), false);

    auto& mods = L.mods().mods();
    for (size_t i = 0; i < mods.size(); ++i) {
        auto& m = mods[i];
        ImVec2 cp = ImGui::GetCursorScreenPos();
        float cw = ImGui::GetContentRegionAvail().x;
        float ch = 90.f;

        ImDrawList* cdl = ImGui::GetWindowDrawList();
        cdl->AddRectFilled(cp, ImVec2(cp.x+cw, cp.y+ch), COL_CARD, 12.f);
        cdl->AddRect(cp, ImVec2(cp.x+cw, cp.y+ch), U32(60,80,120,100), 12.f, 0, 1.f);

        ImGui::SetCursorScreenPos(ImVec2(cp.x+16, cp.y+12));
        ImGui::PushFont(g_fontBold);
        ImGui::Text("%s", m.name.c_str());
        ImGui::PopFont();

        ImGui::SetCursorScreenPos(ImVec2(cp.x+16, cp.y+38));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
        ImGui::Text("v%s  •  %s  •  %s", m.version.c_str(), m.source.c_str(),
                    m.installed ? "installed" : "not installed");
        ImGui::PopStyleColor();

        // Buttons bên phải
        float bx = cp.x + cw - 320;
        ImGui::SetCursorScreenPos(ImVec2(bx, cp.y + 30));
        ImGui::PushStyleColor(ImGuiCol_Text,
            m.enabled ? ImVec4(0.36f,0.75f,0.45f,1.f) : ImVec4(0.7f,0.5f,0.5f,1.f));
        ImGui::Text("%s", m.enabled ? "ON" : "OFF");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button(m.enabled ? "Disable" : "Enable", ImVec2(80, 30)))
            L.mods().setEnabled(i, !m.enabled, Settings::I().d().minecraftDir);
        ImGui::SameLine();
        if (ImGui::Button("Update", ImVec2(80, 30)))
            L.mods().updateAll(Settings::I().d().minecraftDir, nullptr);
        ImGui::SameLine();
        if (ImGui::Button("Delete", ImVec2(80, 30)))
            L.mods().remove(i, Settings::I().d().minecraftDir);

        ImGui::SetCursorScreenPos(ImVec2(cp.x, cp.y+ch+10));
    }

    ImGui::EndChild();
}

// ======================= SETTINGS =======================
static void DrawSettings(Launcher& L, ImVec2 pos, ImVec2 size) {
    (void)L;
    auto& d = Settings::I().d();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    DrawGradientV(dl, pos, ImVec2(pos.x+size.x, pos.y+size.y), COL_BG, COL_BG2, 14.f);

    float pad = 24.f;
    ImGui::PushFont(g_fontBold);
    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad, pos.y+pad));
    ImGui::Text("Settings");
    ImGui::PopFont();

    float cy = pos.y + pad + 50;
    float cw = size.x - pad*2;

    auto Card = [&](float h){
        ImVec2 cp(pos.x+pad, cy);
        dl->AddRectFilled(cp, ImVec2(cp.x+cw, cp.y+h), COL_CARD, 12.f);
        dl->AddRect(cp, ImVec2(cp.x+cw, cp.y+h), U32(60,80,120,80), 12.f, 0, 1.f);
        ImGui::SetCursorScreenPos(ImVec2(cp.x+16, cp.y+14));
    };

    // Card 1
    Card(84);
    ImGui::Text("Minecraft Directory");
    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad+16, cy+44));
    static char dirB[512];
    strncpy_s(dirB, d.minecraftDir.c_str(), sizeof(dirB)-1);
    ImGui::SetNextItemWidth(cw - 32);
    if (ImGui::InputText("##dir", dirB, sizeof(dirB))) {
        d.minecraftDir = dirB; Settings::I().save();
    }
    cy += 92;

    // Card 2
    Card(84);
    ImGui::Text("Java Path");
    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad+16, cy+44));
    static char jB[512];
    strncpy_s(jB, d.javaPath.c_str(), sizeof(jB)-1);
    ImGui::SetNextItemWidth(cw - 180);
    if (ImGui::InputText("##java", jB, sizeof(jB))) {
        d.javaPath = jB; Settings::I().save();
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Detect", ImVec2(140, 0))) {
        d.javaPath = Minecraft::FindJava();
        Settings::I().save();
        strncpy_s(jB, d.javaPath.c_str(), sizeof(jB)-1);
    }
    cy += 92;

    // Card 3
    Card(130);
    ImGui::Text("RAM (MB)");
    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad+16, cy+44));
    ImGui::SetNextItemWidth(240);
    if (ImGui::SliderInt("##ram", &d.ramMB, 512, 16384)) Settings::I().save();

    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad+16, cy+82));
    ImGui::Text("Window");
    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad+90, cy+82));
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("##ww", &d.windowWidth, 0, 0)) Settings::I().save();
    ImGui::SameLine(); ImGui::Text("x"); ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("##wh", &d.windowHeight, 0, 0)) Settings::I().save();
    cy += 138;

    // Card 4
    Card(120);
    if (ImGui::Checkbox("Close launcher after launch", &d.closeAfterLaunch))
        Settings::I().save();
    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad+16, cy+48));
    if (ImGui::Checkbox("Debug Mode", &d.debugMode))
        Settings::I().save();
    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad+16, cy+80));
    if (ImGui::Button("Open RavenXD Folder", ImVec2(200, 26))) {
        std::string p = Settings::I().appDataDir();
        ShellExecuteA(nullptr, "open", p.c_str(), nullptr, nullptr, SW_SHOW);
    }
    ImGui::SetCursorScreenPos(ImVec2(pos.x+pad+240, cy+84));
    bool dc = DiscordRPC::I().isReady();
    ImGui::TextColored(dc ? ImVec4(0.31f,0.78f,0.47f,1.f)
                          : ImVec4(0.6f,0.6f,0.65f,1.f),
                       dc ? "Discord: Connected" : "Discord: Not running");
}

// ======================= JAVA POPUP =======================
static void DrawJavaPopup(Launcher& L) {
    if (L.s().showJavaPopup) ImGui::OpenPopup("Java");
    ImGui::SetNextWindowSize(ImVec2(420, 0));
    if (ImGui::BeginPopupModal("Java", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushFont(g_fontBold);
        ImGui::TextColored(ImVec4(0.95f,0.55f,0.55f,1.f), "Java not found");
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::TextWrapped("RavenXD couldn't locate a Java 8 installation. "
                           "Pick javaw.exe manually or install Java 8 (Temurin).");
        ImGui::Dummy(ImVec2(0, 8));
        if (ImGui::Button("Select Java", ImVec2(160, 34))) {
            char f[MAX_PATH] = {};
            OPENFILENAMEA o{};
            o.lStructSize = sizeof(o);
            o.hwndOwner = GetActiveWindow();
            o.lpstrFilter = "Java\0javaw.exe;java.exe\0All\0*.*\0";
            o.lpstrFile = f; o.nMaxFile = MAX_PATH;
            o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileNameA(&o)) {
                Settings::I().d().javaPath = f;
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

// ======================= LOADING =======================
void UI::RenderLoadingScreen(Launcher& L) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(ImVec2(0,0), io.DisplaySize, COL_BG);

    float t = L.s().loadingTimer;
    float a = t < 0.6f ? t/0.6f : 1.f;
    float sc = 0.85f + 0.15f * (t > 1.0f ? 1.f : t);
    if (sc > 1.f) sc = 1.f;

    float cx = io.DisplaySize.x * 0.5f;
    float cy = io.DisplaySize.y * 0.5f - 30;
    float sz = 120.f * sc;

    ensureLogo();
    if (g_logoTex) {
        dl->AddImage((ImTextureID)g_logoTex,
                     ImVec2(cx-sz*0.5f, cy-sz*0.5f),
                     ImVec2(cx+sz*0.5f, cy+sz*0.5f),
                     ImVec2(0,0), ImVec2(1,1),
                     IM_COL32(255,255,255,(int)(a*255)));
    } else {
        dl->AddText(ImVec2(cx-40, cy-10), IM_COL32(123,167,224,(int)(a*255)), "RavenXD");
    }

    const char* ph = t < 0.8f ? "Checking files..." :
                     t < 1.6f ? "Loading launcher..." : "Ready";
    ImVec2 ts = ImGui::CalcTextSize(ph);
    dl->AddText(ImVec2(cx-ts.x*0.5f, cy+90),
                IM_COL32(170,185,210,(int)(a*255)), ph);
}

// ======================= ROOT =======================
void UI::Render(Launcher& L, float dt) {
    L.tick(dt);

    if (L.s().showLoadingScreen) {
        RenderLoadingScreen(L);
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    // Nền tổng
    DrawGradientV(dl, ImVec2(0,0), io.DisplaySize, COL_BG, COL_BG2, 0.f);

    float sidebarW = 72.f;
    float topH     = 56.f;
    float margin   = 12.f;

    // Sidebar
    DrawSidebar(L, dl, ImVec2(margin, margin + topH + 8),
                sidebarW, io.DisplaySize.y - margin*2 - topH - 8);

    // Topbar
    const char* title = "RavenXD";
    switch (L.s().page) {
        case Page::Home:     title = "RavenXD";    break;
        case Page::Versions: title = "Versions";   break;
        case Page::Mods:     title = "Mods";       break;
        case Page::Settings: title = "Settings";   break;
    }
    DrawTopBar(dl, ImVec2(margin + sidebarW + 8, margin),
               io.DisplaySize.x - margin*2 - sidebarW - 8, topH, title);

    // Content
    ImVec2 cpos(margin + sidebarW + 8, margin + topH + 8);
    ImVec2 csz(io.DisplaySize.x - margin*2 - sidebarW - 8,
               io.DisplaySize.y - margin*2 - topH - 8);

    ImGui::SetNextWindowPos(cpos);
    ImGui::SetNextWindowSize(csz);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::Begin("##content", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground);

    // Đảm bảo font mặc định
    ImGui::PushFont(g_fontRegular);

    switch (L.s().page) {
        case Page::Home:     DrawHome(L, cpos, csz); break;
        case Page::Versions: DrawVersionsPage(L, cpos, csz); break;
        case Page::Mods:     DrawMods(L, cpos, csz); break;
        case Page::Settings: DrawSettings(L, cpos, csz); break;
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();

    DrawJavaPopup(L);
}
