#include <windows.h>
#include "Launcher.h"
#include "DownloadManager.h"
#include "Minecraft.h"
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

Launcher::Launcher() = default;
Launcher::~Launcher() {
    if (m_worker.joinable()) m_worker.join();
}

void Launcher::init() {
    Settings::I().load();
    m_mods.loadFromConfig(Settings::I().d().minecraftDir);
    m_s.javaOk = !Minecraft::FindJava().empty();
    if (!m_s.javaOk) m_s.showJavaPopup = true;
}

void Launcher::setStatus(const std::string& s, float pct, float bps) {
    std::lock_guard<std::mutex> lk(m_mtx);
    m_s.statusText = s;
    m_s.progress = pct;
    m_s.speedMBps = bps;
}

void Launcher::tick(float dt) {
    if (m_s.showLoadingScreen) {
        m_s.loadingTimer += dt;
        m_s.loadingPhase = m_s.loadingTimer / 2.0f;      // 2 sec loading screen
        if (m_s.loadingTimer >= 2.0f) m_s.showLoadingScreen = false;
    }
}

void Launcher::onLaunchClicked() {
    if (m_busy.load()) return;
    if (m_worker.joinable()) m_worker.join();
    m_busy.store(true);
    m_s.taskState = TaskState::Running;
    m_s.lastError.clear();
    m_worker = std::thread([this]{ runLaunchTask(); });
}

void Launcher::runLaunchTask() {
    auto& d = Settings::I().d();

    auto status = [this](const std::string& phase, float pct) {
        setStatus(phase, pct, 0.f);
    };

    // 1) Java check
    std::string java = Minecraft::FindJava();
    if (java.empty()) {
        setStatus("Java not found", 0.f);
        m_s.taskState = TaskState::Failed;
        m_s.lastError = "Java not found";
        m_s.showJavaPopup = true;
        m_busy.store(false);
        return;
    }
    d.javaPath = java;

    // 2) Vanilla
    setStatus("Checking Minecraft 1.8.9...", 0.02f);
    if (!Minecraft::EnsureVanilla(d.minecraftDir, status)) {
        m_s.taskState = TaskState::Failed;
        m_s.lastError = "Failed to install Minecraft 1.8.9";
        m_busy.store(false);
        return;
    }

    std::string versionId = "1.8.9";

    // 3) Forge
    if (m_s.selectedVersion >= 1) {
        setStatus("Checking Forge 1.8.9...", 0.5f);
        std::string forgeId;
        if (!Minecraft::EnsureForge(d.minecraftDir, java, status, forgeId)) {
            m_s.taskState = TaskState::Failed;
            m_s.lastError = "Failed to install Forge 1.8.9";
            m_busy.store(false);
            return;
        }
        versionId = forgeId;
    }

    // 4) OptiFine
    if (m_s.selectedVersion >= 2) {
        setStatus("Checking OptiFine...", 0.8f);
        if (!Minecraft::EnsureOptiFine(d.minecraftDir, status)) {
            m_s.taskState = TaskState::Failed;
            m_s.lastError = "Failed to install OptiFine";
            m_busy.store(false);
            return;
        }
    }

    // 5) Mods
    setStatus("Downloading mods...", 0.9f);
    m_mods.downloadMissing(d.minecraftDir, [this](const std::string& s, float p){
        setStatus(s, 0.9f + 0.05f * p);
    });

    // 6) Launch
    setStatus("Launching Minecraft...", 0.98f);
    auto res = Minecraft::Launch(d.minecraftDir, java, versionId,
                                 d.username, d.ramMB,
                                 d.windowWidth, d.windowHeight);
    if (!res.ok) {
        m_s.taskState = TaskState::Failed;
        m_s.lastError = res.error;
        m_busy.store(false);
        return;
    }

    setStatus("Ready to launch", 1.f);
    m_s.taskState = TaskState::Done;
    m_busy.store(false);

    if (d.closeAfterLaunch) {
        // Signal main loop to exit
        PostQuitMessage(0);
    }
}
