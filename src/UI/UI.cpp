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
ID3D11ShaderResourceView*      g_logoTex = nullptr;
static bool                    g_logoTried = false;

extern void LoadTextureFromMemory(const unsigned char* data, size_t len, ID3D11ShaderResourceView** out);

// ---------- Palette ---------
static ImU32 U32(int r, int g, int b, int a=255){ return IM_COL32(r,g,b,a); }

static const ImU32 COL_BG       = U32(13, 18, 32);
static const ImU32 COL_SIDEBAR  = U32(18, 24, 42);
static const ImU32 COL_TOP      = U32(18, 24, 42);
static const ImU32 COL_CARD     = U32(30, 40, 66);
static const ImU32 COL_TEXT     = U32(232, 238, 248);
static const ImU32 COL_TEXT_DIM = U32(150, 165, 195);
static const ImU32 COL_ACCENT   = U32(91, 141, 214);
static const ImU32 COL_ACCENT_HI= U32(123, 167, 224);

// ================== STATE (dùng static cho đơn giản) ==================
static int  g_detailIndex = -1;          // -1 = không xem chi tiết
static bool g_showLangMenu = false;

// Thông tin từng version (khớp với mapping cũ)
struct VersionInfo {
    const char* shortName;    // "LEGACY 1.12.2"
    const char* subName;      // "Client"
    const char* description;  // đoạn mô tả
    bool        free;
    int         mapToSelected; // → LauncherState::selectedVersion
    // gradient
    int r1,g1,b1, r2,g2,b2;
};

static VersionInfo g_versions[4] = {
    {
        "1.16.5", "Client",
        "Minecraft 1.16.5 - Vanilla client. The classic version "
        "with the Nether Update. Full support for Forge mods and "
        "OptiFine shaders. Recommended for players who want "
        "modern features while keeping stable performance.",
        false, 0,
        120, 40, 60,  180, 60, 30
    },
    {
        "ALPHA 1.16.5", "Client",
        "Alpha build of 1.16.5 for testing new features. This is "
        "an experimental version - bugs may occur. Use it only if "
        "you want to help test upcoming changes and provide feedback.",
        false, 1,
        20, 60, 60,  40, 150, 120
    },
    {
        "LEGACY 1.12.2", "Client",
        "We have prepared a special build for you - the \"nextgen\" "
        "version of our client for 1.12.2. It's a small return to "
        "the classics, created for convenience and pure enjoyment. "
        "There will be no further updates or support - this version "
        "is complete and remains as we intended it to be. However, "
        "it is completely free and available to anyone who wants to "
        "experience those old feelings in a new format.",
        true, 2,
        100, 140, 200,  60, 100, 160
    },
    {
        "1.21.11", "Client",
        "Latest Minecraft version with the newest features. Requires "
        "Java 21 and more RAM. Some mods may not be compatible yet.",
        false, 1,
        40, 100, 50,  100, 160, 80
    }
};

// ================== STYLE ==================
void UI::ApplyStyle() {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding      = 14.f;
    st.ChildRounding       = 14.f;
    st.FrameRounding       = 10.f;
    st.PopupRounding       = 14.f;
    st.ScrollbarRounding   = 12.f;
    st.GrabRounding        = 12.f;
    st.TabRounding         = 10.f;
    st.WindowBorderSize    = 0.f;
    st.FrameBorderSize     = 0.f;
    st.WindowPadding       = ImVec2(0,0);
    st.FramePadding        = ImVec2(12, 8);
    st.ItemSpacing         = ImVec2(10, 10);
    st.ScrollbarSize       = 6.f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg]            = ImVec4(0.05f,0.07f,0.13f,1.f);
    c[ImGuiCol_ChildBg]             = ImVec4(0.07f,0.09f,0.17f,1.f);
    c[ImGuiCol_PopupBg]             = ImVec4(0.08f,0.11f,0.20f,1.f);
    c[ImGuiCol_Border]              = ImVec4(0.20f,0.30f,0.50f,0.25f);
    c[ImGuiCol_FrameBg]             = ImVec4(0.11f,0.15f,0.26f,1.f);
    c[ImGuiCol_FrameBgHovered]      = ImVec4(0.15f,0.21f,0.35f,1.f);
    c[ImGuiCol_FrameBgActive]       = ImVec4(0.18f,0.25f,0.42f,1.f);
    c[ImGuiCol_Button]              = ImVec4(0.18f,0.25f,0.42f,1.f);
    c[ImGuiCol_ButtonHovered]       = ImVec4(0.24f,0.34f,0.55f,1.f);
    c[ImGuiCol_ButtonActive]        = ImVec4(0.30f,0.42f,0.66f,1.f);
    c[ImGuiCol_Header]              = ImVec4(0.20f,0.30f,0.50f,0.7f);
    c[ImGuiCol_HeaderHovered]       = ImVec4(0.26f,0.38f,0.60f,0.9f);
    c[ImGuiCol_HeaderActive]        = ImVec4(0.32f,0.46f,0.72f,1.f);
    c[ImGuiCol_Separator]           = ImVec4(0.20f,0.30f,0.50f,0.35f);
    c[ImGuiCol_Text]                = ImVec4(0.91f,0.93f,0.97f,1.f);
    c[ImGuiCol_TextDisabled]        = ImVec4(0.55f,0.62f,0.75f,1.f);
    c[ImGuiCol_CheckMark]           = ImVec4(0.36f,0.55f,0.84f,1.f);
    c[ImGuiCol_SliderGrab]          = ImVec4(0.36f,0.55f,0.84f,1.f);
    c[ImGuiCol_SliderGrabActive]    = ImVec4(0.48f,0.65f,0.88f,1.f);
    c[ImGuiCol_ScrollbarBg]         = ImVec4(0.f,0.f,0.f,0.f);
    c[ImGuiCol_ScrollbarGrab]       = ImVec4(0.20f,0.30f,0.50f,1.f);
    c[ImGuiCol_ModalWindowDimBg]    = ImVec4(0.0f,0.0f,0.0f,0.55f);
}

static void ensureLogo() {
    if (g_logoTried) return;
    g_logoTried = true;
    if (RAVENXD_LOGO_PNG_SIZE > 4)
        LoadTextureFromMemory(RAVENXD_LOGO_PNG, RAVENXD_LOGO_PNG_SIZE, &g_logoTex);
}

// ================== ICON DRAW HELPERS ==================
static void DrawIconHome(ImDrawList* dl, ImVec2 c, float sz, ImU32 col) {
    dl->AddTriangleFilled(
        ImVec2(c.x, c.y - sz*0.5f),
        ImVec2(c.x - sz*0.55f, c.y),
        ImVec2(c.x + sz*0.55f, c.y), col);
    dl->AddRectFilled(
        ImVec2(c.x - sz*0.38f, c.y),
        ImVec2(c.x + sz*0.38f, c.y + sz*0.55f),
        col, 2.f);
}

static void DrawIconUser(ImDrawList* dl, ImVec2 c, float sz, ImU32 col) {
    dl->AddCircleFilled(ImVec2(c.x, c.y - sz*0.2f), sz*0.28f, col);
    dl->AddCircleFilled(ImVec2(c.x, c.y + sz*0.45f), sz*0.5f, col);
}

static void DrawIconGear(ImDrawList* dl, ImVec2 c, float sz, ImU32 col) {
    dl->AddCircleFilled(c, sz*0.55f, col);
    dl->AddCircleFilled(c, sz*0.25f, COL_SIDEBAR);
    for (int i = 0; i < 8; ++i) {
        float a = i * IM_PI / 4.f;
        float r1 = sz*0.55f, r2 = sz*0.75f;
        ImVec2 p1(c.x + cosf(a)*r1, c.y + sinf(a)*r1);
        ImVec2 p2(c.x + cosf(a)*r2, c.y + sinf(a)*r2);
        dl->AddLine(p1, p2, col, 3.f);
    }
}

static void DrawIconExit(ImDrawList* dl, ImVec2 c, float sz, ImU32 col) {
    dl->AddRect(ImVec2(c.x - sz*0.5f, c.y - sz*0.5f),
                ImVec2(c.x + sz*0.4f, c.y + sz*0.5f),
                col, 3.f, 0, 2.f);
    dl->AddLine(ImVec2(c.x - sz*0.1f, c.y),
                ImVec2(c.x + sz*0.6f, c.y), col, 2.f);
    dl->AddTriangleFilled(
        ImVec2(c.x + sz*0.6f, c.y - sz*0.25f),
        ImVec2(c.x + sz*0.6f, c.y + sz*0.25f),
        ImVec2(c.x + sz*0.85f, c.y), col);
}

// ================== SIDEBAR ==================
static bool SidebarIcon(ImDrawList* dl, ImVec2 pos, float size, bool active,
                        void (*draw)(ImDrawList*, ImVec2, float, ImU32), const char* id)
{
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(id, ImVec2(size, size));
    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();

    ImVec2 c(pos.x + size*0.5f, pos.y + size*0.5f);

    if (active) {
        dl->AddRectFilled(pos, ImVec2(pos.x+size, pos.y+size),
                         U32(45,70,120,180), 12.f);
    } else if (hovered) {
        dl->AddRectFilled(pos, ImVec2(pos.x+size, pos.y+size),
                         U32(30,44,74,180), 12.f);
    }

    ImU32 col = active ? COL_ACCENT : (hovered ? U32(200,215,240) : U32(140,155,185));
    draw(dl, c, size * 0.45f, col);

    return clicked;
}

static void DrawSidebar(Launcher& L, ImDrawList* dl, ImVec2 pos, float w, float h) {
    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), COL_SIDEBAR, 14.f);

    float btnSize = 48.f;
    float x = pos.x + (w - btnSize) * 0.5f;
    float y = pos.y + 24.f;

    // Home
    if (SidebarIcon(dl, ImVec2(x, y), btnSize, L.s().page == Page::Home,
                    DrawIconHome, "##home")) {
        L.s().page = Page::Home;
        g_detailIndex = -1;
    }
    y += btnSize + 12;

    // Versions
    if (SidebarIcon(dl, ImVec2(x, y), btnSize, L.s().page == Page::Versions,
                    DrawIconUser, "##vers")) {
        L.s().page = Page::Versions;
        g_detailIndex = -1;
    }
    y += btnSize + 12;

    // Settings
    if (SidebarIcon(dl, ImVec2(x, y), btnSize, L.s().page == Page::Settings,
                    DrawIconGear, "##set")) {
        L.s().page = Page::Settings;
        g_detailIndex = -1;
    }

    // Discord status indicator (ngay trên Exit)
    {
        float dy = pos.y + h - btnSize - 20 - 42;
        ImVec2 dc(x + btnSize*0.5f, dy + btnSize*0.5f);
        bool connected = DiscordRPC::I().isReady();
        ImU32 dotCol = connected ? U32(80, 200, 120) : U32(120, 120, 130);
        dl->AddCircleFilled(dc, btnSize*0.18f, dotCol);
        if (connected) {
            dl->AddCircle(dc, btnSize*0.24f, U32(80, 200, 120, 100), 0, 2.f);
        }
        ImGui::SetCursorScreenPos(ImVec2(x, dy));
        ImGui::InvisibleButton("##discord", ImVec2(btnSize, btnSize));
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextColored(connected
                ? ImVec4(0.31f,0.78f,0.47f,1.f)
                : ImVec4(0.6f,0.6f,0.65f,1.f),
                connected ? "Discord: Connected" : "Discord: Not running");
            ImGui::EndTooltip();
        }
    }

    // Exit
    float exY = pos.y + h - btnSize - 20;
    if (SidebarIcon(dl, ImVec2(x, exY), btnSize, false,
                    DrawIconExit, "##exit")) {
        PostQuitMessage(0);
    }
}

// ================== TOP BAR ==================
static void DrawTopBar(ImDrawList* dl, ImVec2 pos, float w, float h) {
    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), COL_TOP, 14.f);

    // Logo nhỏ
    ensureLogo();
    float logoSize = 28.f;
    ImVec2 logoPos(pos.x + 18, pos.y + (h - logoSize)*0.5f);
    if (g_logoTex) {
        dl->AddImage((ImTextureID)g_logoTex,
                     logoPos, ImVec2(logoPos.x + logoSize, logoPos.y + logoSize));
    }

    // Title "RavenXD" in đậm
    ImVec2 titlePos(logoPos.x + logoSize + 12, pos.y + (h - 20)*0.5f);
    dl->AddText(titlePos, COL_TEXT, "RavenXD");

    // Language selector (giả lập — click vào hiện menu)
    float langW = 110.f;
    ImVec2 langPos(titlePos.x + 90, pos.y + (h - 32)*0.5f);
    dl->AddRectFilled(langPos,
                     ImVec2(langPos.x + langW, langPos.y + 32),
                     U32(30, 40, 66), 10.f);
    // Cờ US (đơn giản: hình chữ nhật đỏ + trắng)
    ImVec2 flagPos(langPos.x + 10, langPos.y + 9);
    dl->AddRectFilled(flagPos, ImVec2(flagPos.x + 18, flagPos.y + 14), U32(200,40,50));
    for (int i = 0; i < 3; ++i) {
        dl->AddRectFilled(ImVec2(flagPos.x, flagPos.y + i*5 + 2),
                         ImVec2(flagPos.x + 18, flagPos.y + i*5 + 4),
                         U32(255,255,255));
    }
    dl->AddText(ImVec2(flagPos.x + 26, langPos.y + 8), COL_TEXT, "English");
    // Mũi tên xuống
    ImVec2 ar(langPos.x + langW - 14, langPos.y + 15);
    dl->AddTriangleFilled(ImVec2(ar.x - 4, ar.y - 2),
                          ImVec2(ar.x + 4, ar.y - 2),
                          ImVec2(ar.x, ar.y + 4), COL_TEXT_DIM);

    ImGui::SetCursorScreenPos(langPos);
    ImGui::InvisibleButton("##lang", ImVec2(langW, 32));
    if (ImGui::IsItemClicked()) g_showLangMenu = !g_showLangMenu;

    if (g_showLangMenu) {
        ImGui::SetNextWindowPos(ImVec2(langPos.x, langPos.y + 36));
        ImGui::SetNextWindowSize(ImVec2(langW, 0));
        ImGui::Begin("##langpop", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
        if (ImGui::Selectable("English")) g_showLangMenu = false;
        if (ImGui::Selectable("Tiếng Việt")) g_showLangMenu = false;
        ImGui::End();
    }

    // Minimize + Close buttons
    float btnW = 46.f, btnH = h;
    ImVec2 minPos(pos.x + w - btnW*2, pos.y);
    ImVec2 closePos(pos.x + w - btnW, pos.y);

    // Minimize
    ImGui::SetCursorScreenPos(minPos);
    ImGui::InvisibleButton("##min", ImVec2(btnW, btnH));
    bool minHover = ImGui::IsItemHovered();
    if (minHover)
        dl->AddRectFilled(minPos, ImVec2(minPos.x+btnW, minPos.y+btnH),
                         U32(45,60,90,180));
    dl->AddLine(ImVec2(minPos.x + btnW*0.35f, minPos.y + btnH*0.5f),
                ImVec2(minPos.x + btnW*0.65f, minPos.y + btnH*0.5f),
                minHover ? COL_TEXT : COL_TEXT_DIM, 1.8f);
    if (ImGui::IsItemClicked())
        ShowWindow(GetActiveWindow(), SW_MINIMIZE);

    // Close
    ImGui::SetCursorScreenPos(closePos);
    ImGui::InvisibleButton("##close", ImVec2(btnW, btnH));
    bool closeHover = ImGui::IsItemHovered();
    if (closeHover)
        dl->AddRectFilled(closePos, ImVec2(closePos.x+btnW, closePos.y+btnH),
                         U32(220,60,60,200));
    ImU32 xCol = closeHover ? U32(255,255,255) : COL_TEXT_DIM;
    dl->AddLine(ImVec2(closePos.x + btnW*0.35f, closePos.y + btnH*0.35f),
                ImVec2(closePos.x + btnW*0.65f, closePos.y + btnH*0.65f), xCol, 1.8f);
    dl->AddLine(ImVec2(closePos.x + btnW*0.65f, closePos.y + btnH*0.35f),
                ImVec2(closePos.x + btnW*0.35f, closePos.y + btnH*0.65f), xCol, 1.8f);
    if (ImGui::IsItemClicked())
        PostQuitMessage(0);
}

// ================== VERSION CARD (grid) ==================
static bool DrawVersionCard(ImVec2 pos, ImVec2 size, const VersionInfo& v,
                             bool selected)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton(v.shortName, size);
    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();

    // Gradient background
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     U32(v.r1, v.g1, v.b1), 14.f);
    int steps = 20;
    for (int i = 0; i < steps; ++i) {
        float t = (float)i / steps;
        float y1 = pos.y + size.y * (0.3f + 0.7f * t);
        float y2 = pos.y + size.y * (0.3f + 0.7f * (t + 1.f/steps));
        int rr = v.r1 + (int)((v.r2 - v.r1) * t);
        int gg = v.g1 + (int)((v.g2 - v.g1) * t);
        int bb = v.b1 + (int)((v.b2 - v.b1) * t);
        dl->AddRectFilled(ImVec2(pos.x, y1), ImVec2(pos.x + size.x, y2),
                         U32(rr, gg, bb));
    }
    // Bo góc 4 góc
    dl->AddRectFilled(pos, ImVec2(pos.x + 14, pos.y + size.y), U32(v.r1, v.g1, v.b1));
    dl->AddRectFilled(ImVec2(pos.x + size.x - 14, pos.y),
                      ImVec2(pos.x + size.x, pos.y + size.y), U32(v.r2, v.g2, v.b2));

    // Overlay tối
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     U32(0, 0, 0, hovered ? 40 : 90), 14.f);

    // Viền
    if (selected) {
        dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                    COL_ACCENT, 14.f, 0, 3.f);
    } else if (hovered) {
        dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                    U32(120, 160, 220, 200), 14.f, 0, 2.f);
    }

    // Tag "Free" / "Client"
    const char* tag = v.free ? "Free" : "Client";
    ImVec2 tagSize = ImGui::CalcTextSize(tag);
    float tagPadX = 10.f, tagPadY = 5.f;
    ImVec2 tagPos(pos.x + size.x - tagSize.x - tagPadX*2 - 10, pos.y + 10);
    ImU32 tagBg = v.free ? U32(70, 170, 90) : U32(30, 40, 66, 230);
    dl->AddRectFilled(tagPos,
                     ImVec2(tagPos.x + tagSize.x + tagPadX*2,
                            tagPos.y + tagSize.y + tagPadY*2),
                     tagBg, 6.f);
    dl->AddText(ImVec2(tagPos.x + tagPadX, tagPos.y + tagPadY),
                U32(255,255,255), tag);

    // Title dưới trái
    ImVec2 titleSize = ImGui::CalcTextSize(v.shortName);
    ImVec2 titlePos(pos.x + 14, pos.y + size.y - titleSize.y - 20);
    dl->AddText(ImVec2(titlePos.x + 1, titlePos.y + 1), U32(0,0,0,180), v.shortName);
    dl->AddText(titlePos, U32(255,255,255), v.shortName);

    // Mũi tên phải dưới
    float arrowSz = 18.f;
    ImVec2 arrowCenter(pos.x + size.x - 22, pos.y + size.y - 22);
    ImU32 arrowCol = hovered ? COL_ACCENT : U32(255,255,255,200);
    if (hovered)
        dl->AddCircleFilled(arrowCenter, arrowSz * 0.9f, U32(91,141,214,80));
    float al = 6.f;
    dl->AddLine(ImVec2(arrowCenter.x - al, arrowCenter.y),
                ImVec2(arrowCenter.x + al, arrowCenter.y), arrowCol, 2.f);
    dl->AddLine(ImVec2(arrowCenter.x + al, arrowCenter.y),
                ImVec2(arrowCenter.x + al*0.3f, arrowCenter.y - al*0.6f), arrowCol, 2.f);
    dl->AddLine(ImVec2(arrowCenter.x + al, arrowCenter.y),
                ImVec2(arrowCenter.x + al*0.3f, arrowCenter.y + al*0.6f), arrowCol, 2.f);

    return clicked;
}

// ================== VERSIONS GRID PAGE ==================
static void DrawVersionsGrid(Launcher& L, ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     U32(13, 18, 32), 14.f);

    float padX = 24.f, padY = 24.f;
    float gap = 16.f;
    int cols = 3;
    float cardW = (size.x - padX*2 - gap*(cols-1)) / cols;
    float cardH = 160.f;

    ImVec2 start(pos.x + padX, pos.y + padY);

    for (int i = 0; i < 4; ++i) {
        int row = i / cols;
        int col = i % cols;
        ImVec2 cpos(start.x + col * (cardW + gap),
                    start.y + row * (cardH + gap));
        if (cpos.x + cardW > pos.x + size.x - padX) continue;

        bool selected = (L.s().selectedVersion == g_versions[i].mapToSelected);

        if (DrawVersionCard(cpos, ImVec2(cardW, cardH), g_versions[i], selected)) {
            // Mở trang chi tiết
            g_detailIndex = i;
        }
    }
}

// ================== VERSION DETAIL PAGE (giống ảnh) ==================
static void DrawVersionDetail(Launcher& L, ImVec2 pos, ImVec2 size) {
    if (g_detailIndex < 0 || g_detailIndex >= 4) {
        g_detailIndex = -1;
        return;
    }
    const VersionInfo& v = g_versions[g_detailIndex];
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     U32(13, 18, 32), 14.f);

    float pad = 24.f;

    // ===== CARD ẢNH BÊN TRÁI =====
    float cardW = size.x * 0.40f;
    float cardH = size.y - pad*2;
    ImVec2 cardPos(pos.x + pad, pos.y + pad);

    // Gradient background card
    dl->AddRectFilled(cardPos, ImVec2(cardPos.x + cardW, cardPos.y + cardH),
                     U32(v.r1, v.g1, v.b1), 14.f);
    int steps = 24;
    for (int i = 0; i < steps; ++i) {
        float t = (float)i / steps;
        float y1 = cardPos.y + cardH * t;
        float y2 = cardPos.y + cardH * (t + 1.f/steps);
        int rr = v.r1 + (int)((v.r2 - v.r1) * t);
        int gg = v.g1 + (int)((v.g2 - v.g1) * t);
        int bb = v.b1 + (int)((v.b2 - v.b1) * t);
        dl->AddRectFilled(ImVec2(cardPos.x, y1), ImVec2(cardPos.x + cardW, y2),
                         U32(rr, gg, bb));
    }
    // Bo 4 góc
    dl->AddRectFilled(cardPos, ImVec2(cardPos.x + 14, cardPos.y + cardH),
                     U32(v.r1, v.g1, v.b1));
    dl->AddRectFilled(ImVec2(cardPos.x + cardW - 14, cardPos.y),
                      ImVec2(cardPos.x + cardW, cardPos.y + cardH),
                      U32(v.r2, v.g2, v.b2));
    // Overlay nhẹ
    dl->AddRectFilled(cardPos, ImVec2(cardPos.x + cardW, cardPos.y + cardH),
                     U32(0,0,0,60), 14.f);

    // Tag "Free" góc phải trên
    if (v.free) {
        const char* tag = "Free";
        ImVec2 tagSize = ImGui::CalcTextSize(tag);
        float tpx = 10.f, tpy = 5.f;
        ImVec2 tpos(cardPos.x + cardW - tagSize.x - tpx*2 - 12, cardPos.y + 12);
        dl->AddRectFilled(tpos,
                         ImVec2(tpos.x + tagSize.x + tpx*2, tpos.y + tagSize.y + tpy*2),
                         U32(70, 170, 90), 6.f);
        dl->AddText(ImVec2(tpos.x + tpx, tpos.y + tpy), U32(255,255,255), tag);
    }

    // Title dưới cùng card
    ImVec2 titleSize = ImGui::CalcTextSize(v.shortName);
    ImVec2 titlePos(cardPos.x + 18, cardPos.y + cardH - titleSize.y - 18);
    dl->AddText(ImVec2(titlePos.x + 1, titlePos.y + 1), U32(0,0,0,200), v.shortName);
    dl->AddText(titlePos, U32(255,255,255), v.shortName);

    // ===== NỘI DUNG BÊN PHẢI =====
    float rightX = cardPos.x + cardW + pad;
    float rightW = size.x - (rightX - pos.x) - pad;

    // Title lớn "LEGACY 1.12.2"
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.91f,0.93f,0.97f,1.f));
    ImGui::SetCursorScreenPos(ImVec2(rightX, pos.y + pad + 4));
    ImGui::SetWindowFontScale(1.8f);
    ImGui::Text("%s", v.shortName);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Nút "RETURN →" góc phải trên
    {
        const char* retLabel = "RETURN  →";
        ImVec2 rs = ImGui::CalcTextSize(retLabel);
        float rw = rs.x + 24.f;
        float rh = 34.f;
        ImVec2 rpos(pos.x + size.x - pad - rw, pos.y + pad + 12);

        ImGui::SetCursorScreenPos(rpos);
        ImGui::InvisibleButton("##return", ImVec2(rw, rh));
        bool hov = ImGui::IsItemHovered();
        if (hov) {
            dl->AddRectFilled(rpos, ImVec2(rpos.x+rw, rpos.y+rh),
                             U32(40, 55, 90), 10.f);
        }
        ImU32 col = hov ? COL_ACCENT_HI : COL_TEXT_DIM;
        dl->AddText(ImVec2(rpos.x + 12, rpos.y + 8), col, retLabel);
        if (ImGui::IsItemClicked()) {
            g_detailIndex = -1;
        }
    }

    // Subtitle "Client"
    ImGui::SetCursorScreenPos(ImVec2(rightX, pos.y + pad + 50));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
    ImGui::SetWindowFontScale(1.1f);
    ImGui::Text("%s", v.subName);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Description (word wrap)
    ImGui::SetCursorScreenPos(ImVec2(rightX, pos.y + pad + 90));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f,0.87f,0.95f,1.f));
    ImGui::PushTextWrapPos(rightX + rightW);
    ImGui::TextWrapped("%s", v.description);
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();

    // ===== NÚT LAUNCH góc phải dưới =====
    float bw = 140.f, bh = 44.f;
    ImVec2 bpos(pos.x + size.x - pad - bw, pos.y + size.y - pad - bh);

    // Glow
    dl->AddRectFilled(ImVec2(bpos.x - 6, bpos.y - 6),
                     ImVec2(bpos.x + bw + 6, bpos.y + bh + 6),
                     U32(91,141,214,60), 20.f);

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.36f,0.55f,0.84f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f,0.65f,0.88f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f,0.45f,0.75f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.f);

    ImGui::SetCursorScreenPos(bpos);
    bool busy = L.s().taskState == TaskState::Running;

    if (!busy) {
        ImGui::SetWindowFontScale(1.15f);
        if (ImGui::Button("  >  LAUNCH", ImVec2(bw, bh))) {
            // Set selected version rồi launch
            L.s().selectedVersion = v.mapToSelected;
            Settings::I().d().version = v.shortName;
            Settings::I().save();
            L.onLaunchClicked();
        }
        ImGui::SetWindowFontScale(1.0f);
    } else {
        ImGui::BeginDisabled();
        ImGui::SetWindowFontScale(1.15f);
        ImGui::Button("  ... WORKING", ImVec2(bw, bh));
        ImGui::SetWindowFontScale(1.0f);
        ImGui::EndDisabled();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
}

// ================== HOME PAGE ==================
static void DrawHome(Launcher& L, ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     U32(13, 18, 32), 14.f);

    float pad = 32.f;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.91f,0.93f,0.97f,1.f));
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad));
    ImGui::SetWindowFontScale(1.8f);
    ImGui::Text("RavenXD");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 44));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.62f,0.75f,1.f));
    ImGui::Text("Minecraft 1.8.9  •  Forge  •  OptiFine");
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 90));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
    ImGui::Text("Profile");
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 115));
    static char userBuf[64];
    static bool init = false;
    if (!init) {
        strncpy_s(userBuf, Settings::I().d().username.c_str(), sizeof(userBuf)-1);
        init = true;
    }
    ImGui::SetNextItemWidth(280);
    if (ImGui::InputText("##user", userBuf, sizeof(userBuf))) {
        Settings::I().d().username = userBuf;
        Settings::I().save();
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 165));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
    ImGui::Text("Version");
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 190));
    const char* versions[] = { "Minecraft 1.8.9", "Forge 1.8.9", "Forge 1.8.9 + OptiFine" };
    ImGui::SetNextItemWidth(380);
    ImGui::Combo("##ver", &L.s().selectedVersion, versions, 3);

    float bw = 360, bh = 62;
    ImVec2 bpos(pos.x + pad, pos.y + pad + 270);

    ImGui::SetCursorScreenPos(bpos);
    bool busy = L.s().taskState == TaskState::Running;

    dl->AddRectFilled(ImVec2(bpos.x - 8, bpos.y - 8),
                     ImVec2(bpos.x + bw + 8, bpos.y + bh + 8),
                     U32(91,141,214,60), 24.f);

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.36f,0.55f,0.84f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.48f,0.65f,0.88f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f,0.45f,0.75f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.f);

    ImGui::SetWindowFontScale(1.5f);
    if (!busy) {
        if (ImGui::Button("   >  LAUNCH", ImVec2(bw, bh)))
            L.onLaunchClicked();
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("   ...  WORKING", ImVec2(bw, bh));
        ImGui::EndDisabled();
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 360));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.36f,0.55f,0.84f,1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0.11f,0.15f,0.26f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f);
    float pct = L.s().progress;
    char overlay[64];
    if (L.s().speedMBps > 0.f)
        snprintf(overlay, sizeof(overlay), "%d%%   %.2f MB/s", (int)(pct*100.f), L.s().speedMBps);
    else
        snprintf(overlay, sizeof(overlay), "%d%%", (int)(pct*100.f));
    ImGui::ProgressBar(pct, ImVec2(size.x - pad*2, 20), overlay);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 395));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
    ImGui::Text("%s", L.s().statusText.c_str());
    ImGui::PopStyleColor();

    if (L.s().taskState == TaskState::Failed && !L.s().lastError.empty()) {
        ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 420));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f,0.45f,0.45f,1.f));
        ImGui::Text("Error: %s", L.s().lastError.c_str());
        ImGui::PopStyleColor();
    }
}

// ================== MODS PAGE ==================
static void DrawMods(Launcher& L, ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     U32(13, 18, 32), 14.f);

    float pad = 24.f;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.91f,0.93f,0.97f,1.f));
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Mods");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x - 150, pos.y + pad + 6));
    if (ImGui::Button("Update All", ImVec2(120, 34))) {
        L.mods().updateAll(Settings::I().d().minecraftDir, nullptr);
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad + 60));

    ImGui::BeginChild("##modscroll", ImVec2(size.x - pad*2, size.y - pad*2 - 60), false);

    auto& mods = L.mods().mods();
    for (size_t i = 0; i < mods.size(); ++i) {
        auto& m = mods[i];
        ImVec2 cpos = ImGui::GetCursorScreenPos();
        float cw = ImGui::GetContentRegionAvail().x;
        float ch = 100.f;

        ImDrawList* cdl = ImGui::GetWindowDrawList();
        cdl->AddRectFilled(cpos, ImVec2(cpos.x + cw, cpos.y + ch),
                          U32(28, 38, 66), 14.f);
        cdl->AddRect(cpos, ImVec2(cpos.x + cw, cpos.y + ch),
                    U32(50, 70, 110, 100), 14.f, 0, 1.f);

        ImGui::SetCursorScreenPos(ImVec2(cpos.x + 18, cpos.y + 14));
        ImGui::SetWindowFontScale(1.15f);
        ImGui::Text("%s", m.name.c_str());
        ImGui::SetWindowFontScale(1.0f);

        ImGui::SetCursorScreenPos(ImVec2(cpos.x + 18, cpos.y + 42));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f,0.70f,0.82f,1.f));
        ImGui::Text("Version %s  •  %s", m.version.c_str(), m.source.c_str());
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos(ImVec2(cpos.x + 18, cpos.y + 66));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f,0.55f,0.72f,1.f));
        ImGui::Text("%s", m.installed ? "Installed" : "Not installed");
        ImGui::PopStyleColor();

        float bx = cpos.x + cw - 320;
        ImGui::SetCursorScreenPos(ImVec2(bx, cpos.y + 34));
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

        ImGui::SetCursorScreenPos(ImVec2(cpos.x, cpos.y + ch + 10));
    }

    ImGui::EndChild();
}

// ================== SETTINGS PAGE ==================
static void DrawSettings(Launcher& L, ImVec2 pos, ImVec2 size) {
    auto& d = Settings::I().d();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     U32(13, 18, 32), 14.f);

    float pad = 24.f;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.91f,0.93f,0.97f,1.f));
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad, pos.y + pad));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Settings");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    float cy = pos.y + pad + 60;
    float cw = size.x - pad*2;

    auto card = [&](float h){
        ImVec2 cpos(pos.x + pad, cy);
        dl->AddRectFilled(cpos, ImVec2(cpos.x + cw, cpos.y + h),
                         U32(28, 38, 66), 14.f);
        ImGui::SetCursorScreenPos(ImVec2(cpos.x + 16, cpos.y + 14));
    };

    // Card 1: Minecraft dir
    card(84);
    ImGui::Text("Minecraft Directory");
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad + 16, cy + 44));
    static char dirBuf[512];
    strncpy_s(dirBuf, d.minecraftDir.c_str(), sizeof(dirBuf)-1);
    ImGui::SetNextItemWidth(cw - 32);
    if (ImGui::InputText("##dir", dirBuf, sizeof(dirBuf))) {
        d.minecraftDir = dirBuf;
        Settings::I().save();
    }
    cy += 94;

    // Card 2: Java
    card(84);
    ImGui::Text("Java Path");
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad + 16, cy + 44));
    static char javaBuf[512];
    strncpy_s(javaBuf, d.javaPath.c_str(), sizeof(javaBuf)-1);
    ImGui::SetNextItemWidth(cw - 180);
    if (ImGui::InputText("##java", javaBuf, sizeof(javaBuf))) {
        d.javaPath = javaBuf;
        Settings::I().save();
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto Detect", ImVec2(140, 0))) {
        d.javaPath = Minecraft::FindJava();
        Settings::I().save();
        strncpy_s(javaBuf, d.javaPath.c_str(), sizeof(javaBuf)-1);
    }
    cy += 94;

    // Card 3: RAM + window
    card(130);
    ImGui::Text("RAM (MB)");
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad + 16, cy + 44));
    ImGui::SetNextItemWidth(240);
    if (ImGui::SliderInt("##ram", &d.ramMB, 512, 16384)) Settings::I().save();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad + 16, cy + 80));
    ImGui::Text("Window");
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad + 90, cy + 80));
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("##ww", &d.windowWidth, 0, 0)) Settings::I().save();
    ImGui::SameLine(); ImGui::Text("x"); ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("##wh", &d.windowHeight, 0, 0)) Settings::I().save();
    cy += 140;

    // Card 4: toggles
    card(110);
    if (ImGui::Checkbox("Close launcher after launch", &d.closeAfterLaunch))
        Settings::I().save();
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad + 16, cy + 50));
    if (ImGui::Checkbox("Debug Mode", &d.debugMode))
        Settings::I().save();
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad + 16, cy + 80));
    if (ImGui::Button("Open RavenXD Folder", ImVec2(200, 26))) {
        std::string p = Settings::I().appDataDir();
        ShellExecuteA(nullptr, "open", p.c_str(), nullptr, nullptr, SW_SHOW);
    }
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad + 240, cy + 82));
    bool dc = DiscordRPC::I().isReady();
    ImGui::TextColored(dc ? ImVec4(0.31f,0.78f,0.47f,1.f)
                          : ImVec4(0.6f,0.6f,0.65f,1.f),
                       dc ? "Discord: Connected" : "Discord: Not running");
}

// ================== JAVA POPUP ==================
static void DrawJavaPopup(Launcher& L) {
    if (L.s().showJavaPopup) ImGui::OpenPopup("Java");
    ImGui::SetNextWindowSize(ImVec2(420, 0));
    if (ImGui::BeginPopupModal("Java", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.95f,0.55f,0.55f,1.f), "Java not found");
        ImGui::Separator();
        ImGui::TextWrapped("RavenXD couldn't locate a Java 8 installation. "
                           "Please pick your javaw.exe manually, or install Java 8 (Adoptium Temurin 8).");
        ImGui::Dummy(ImVec2(0, 8));
        if (ImGui::Button("Install / Select Java", ImVec2(180, 34))) {
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

// ================== LOADING SCREEN ==================
void UI::RenderLoadingScreen(Launcher& L) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(ImVec2(0,0), io.DisplaySize, COL_BG);

    float t = L.s().loadingTimer;
    float alpha = t < 0.6f ? t/0.6f : 1.f;
    float scale = 0.85f + 0.15f * (t > 1.0f ? 1.f : t);
    if (scale > 1.f) scale = 1.f;

    float cx = io.DisplaySize.x * 0.5f;
    float cy = io.DisplaySize.y * 0.5f - 30;

    float sz = 120.f * scale;
    ensureLogo();
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

// ================== ROOT RENDER ==================
void UI::Render(Launcher& L, float dt) {
    L.tick(dt);

    if (L.s().showLoadingScreen) {
        RenderLoadingScreen(L);
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(ImVec2(0,0), io.DisplaySize, COL_BG);

    float sidebarW  = 72.f;
    float topbarH   = 56.f;
    float margin    = 12.f;

    // Sidebar
    DrawSidebar(L, dl, ImVec2(margin, margin + topbarH + 8),
                sidebarW, io.DisplaySize.y - margin*2 - topbarH - 8);

    // Topbar
    DrawTopBar(dl, ImVec2(margin + sidebarW + 8, margin),
               io.DisplaySize.x - margin*2 - sidebarW - 8, topbarH);

    // Content area
    ImVec2 contentPos(margin + sidebarW + 8, margin + topbarH + 8);
    ImVec2 contentSize(io.DisplaySize.x - margin*2 - sidebarW - 8,
                       io.DisplaySize.y - margin*2 - topbarH - 8);

    ImGui::SetNextWindowPos(contentPos);
    ImGui::SetNextWindowSize(contentSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::Begin("##content", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground);

    // Nếu đang mở detail view → ưu tiên vẽ detail (trên page Versions)
    if (g_detailIndex >= 0) {
        DrawVersionDetail(L, contentPos, contentSize);
    } else {
        switch (L.s().page) {
            case Page::Home:     DrawHome(L, contentPos, contentSize); break;
            case Page::Versions: DrawVersionsGrid(L, contentPos, contentSize); break;
            case Page::Mods:     DrawMods(L, contentPos, contentSize); break;
            case Page::Settings: DrawSettings(L, contentPos, contentSize); break;
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();

    DrawJavaPopup(L);
}
