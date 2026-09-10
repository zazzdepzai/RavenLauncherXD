#pragma once
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include "Minecraft.h"
#include "ModManager.h"
#include "Settings.h"

enum class Page { Home, Accounts, Versions, Mods, Settings };
enum class TaskState { Idle, Running, Done, Failed };

struct LauncherState {
    Page page = Page::Home;
    std::string statusText = "Ready";
    float progress = 0.f;
    float speedMBps = 0.f;

    TaskState taskState = TaskState::Idle;
    std::string lastError;

    // Version selection
    int selectedVersion = 2;     // 0=Vanilla, 1=Forge, 2=Forge+OptiFine
    bool showJavaPopup = false;
    bool javaOk = false;

    bool showLoadingScreen = true;
    float loadingTimer = 0.f;
    float loadingPhase = 0.f;
};

class Launcher {
public:
    Launcher();
    ~Launcher();

    void init();
    void tick(float dt);
    void onLaunchClicked();

    LauncherState& s() { return m_s; }
    ModManager& mods() { return m_mods; }

private:
    void runLaunchTask();
    void setStatus(const std::string& s, float pct, float bps = 0.f);

    LauncherState m_s;
    ModManager m_mods;
    std::thread m_worker;
    std::atomic<bool> m_busy{false};
    std::mutex m_mtx;
};
