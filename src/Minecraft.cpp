#include "Minecraft.h"
#include "DownloadManager.h"
#include "Settings.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;
using json = nlohmann::json;

static const char* MANIFEST_URL =
    "https://launchermeta.mojang.com/mc/game/version_manifest_v2.json";
static const char* RESOURCES_BASE =
    "https://resources.download.minecraft.net/";

static std::string joinPath(const std::string& a, const std::string& b) {
    return (fs::path(a) / b).string();
}

// ---------- Java discovery ----------
std::string Minecraft::FindJava() {
    auto& d = Settings::I().d();
    if (!d.javaPath.empty() && DownloadManager::FileExists(d.javaPath)) return d.javaPath;

    char buf[MAX_PATH];
    DWORD n = GetEnvironmentVariableA("JAVA_HOME", buf, MAX_PATH);
    if (n > 0) {
        std::string p = joinPath(buf, "bin\\javaw.exe");
        if (DownloadManager::FileExists(p)) return p;
        p = joinPath(buf, "bin\\java.exe");
        if (DownloadManager::FileExists(p)) return p;
    }
    // Common install paths
    const char* guesses[] = {
        "C:\\Program Files\\Java\\jre1.8.0_401\\bin\\javaw.exe",
        "C:\\Program Files\\Java\\jre1.8.0_391\\bin\\javaw.exe",
        "C:\\Program Files (x86)\\Java\\jre1.8.0_401\\bin\\javaw.exe",
        "C:\\Program Files\\Eclipse Adoptium\\jdk-8.0.402.6-hotspot\\bin\\javaw.exe",
        "C:\\Program Files\\Microsoft\\jdk-8.0.402.6-hotspot\\bin\\javaw.exe",
    };
    for (auto g : guesses) if (DownloadManager::FileExists(g)) return g;

    // PATH
    char pathBuf[32768];
    if (GetEnvironmentVariableA("PATH", pathBuf, sizeof(pathBuf))) {
        std::stringstream ss(pathBuf);
        std::string seg;
        while (std::getline(ss, seg, ';')) {
            std::string p = joinPath(seg, "javaw.exe");
            if (DownloadManager::FileExists(p)) return p;
            p = joinPath(seg, "java.exe");
            if (DownloadManager::FileExists(p)) return p;
        }
    }
    return "";
}

// ---------- Vanilla install ----------
static bool fetchManifest(json& out) {
    std::string body;
    if (!DownloadManager::DownloadToString(MANIFEST_URL, body)) return false;
    try { out = json::parse(body); } catch (...) { return false; }
    return true;
}

static bool fetchVersionJson(const std::string& url, json& out) {
    std::string body;
    if (!DownloadManager::DownloadToString(url, body)) return false;
    try { out = json::parse(body); } catch (...) { return false; }
    return true;
}

static void downloadAllLibraries(const std::string& mcDir, const json& vjson, Minecraft::StatusFn st, float base, float span) {
    if (!vjson.contains("libraries")) return;
    auto libs = vjson["libraries"];
    int total = (int)libs.size();
    int done = 0;
    for (auto& lib : libs) {
        // Skip if rules disallow
        if (lib.contains("rules")) {
            bool allow = false;
            for (auto& r : lib["rules"]) {
                bool osMatch = true;
                if (r.contains("os")) {
                    std::string osName = r["os"].value("name","");
                    if (!osName.empty() && osName != "windows") osMatch = false;
                }
                if (osMatch) allow = (r.value("action","allow") == "allow");
            }
            if (!allow) { done++; continue; }
        }
        // natives
        if (lib.contains("natives")) {
            std::string cls = lib["natives"].value("windows","");
            if (!cls.empty() && lib.contains("downloads") && lib["downloads"].contains("classifiers") &&
                lib["downloads"]["classifiers"].contains(cls)) {
                auto& nv = lib["downloads"]["classifiers"][cls];
                std::string rel = nv.value("path","");
                std::string url = nv.value("url","");
                std::string dst = joinPath(mcDir, "libraries/" + rel);
                if (!DownloadManager::FileExists(dst) && !url.empty()) {
                    DownloadManager::Download(url, dst);
                }
            }
        }
        if (lib.contains("downloads") && lib["downloads"].contains("artifact")) {
            auto& art = lib["downloads"]["artifact"];
            std::string rel = art.value("path","");
            std::string url = art.value("url","");
            std::string dst = joinPath(mcDir, "libraries/" + rel);
            if (!DownloadManager::FileExists(dst) && !url.empty()) {
                DownloadManager::Download(url, dst);
            }
        }
        done++;
        if (st) st("Downloading libraries...", base + span * (float)done / std::max(1,total));
    }
}

static void downloadAssets(const std::string& mcDir, const json& vjson, Minecraft::StatusFn st, float base, float span) {
    if (!vjson.contains("assetIndex")) return;
    auto& ai = vjson["assetIndex"];
    std::string idxUrl  = ai.value("url","");
    std::string idxId   = ai.value("id","");
    std::string idxDst  = joinPath(mcDir, "assets/indexes/" + idxId + ".json");
    if (!DownloadManager::FileExists(idxDst)) {
        DownloadManager::EnsureDir(joinPath(mcDir, "assets/indexes"));
        if (!idxUrl.empty()) DownloadManager::Download(idxUrl, idxDst);
    }
    std::ifstream f(idxDst);
    if (!f) return;
    json idx; try { f >> idx; } catch (...) { return; }
    if (!idx.contains("objects")) return;
    auto& objs = idx["objects"];
    int total = (int)objs.size(), done = 0;
    for (auto it = objs.begin(); it != objs.end(); ++it) {
        std::string hash = it.value().value("hash","");
        if (hash.size() < 2) { done++; continue; }
        std::string sub = hash.substr(0, 2);
        std::string url = std::string(RESOURCES_BASE) + sub + "/" + hash;
        std::string dst = joinPath(mcDir, "assets/objects/" + sub + "/" + hash);
        if (!DownloadManager::FileExists(dst)) {
            DownloadManager::Download(url, dst);
        }
        done++;
        if ((done % 50) == 0 && st) st("Downloading assets...", base + span * (float)done / std::max(1,total));
    }
}

bool Minecraft::EnsureVanilla(const std::string& mcDir, StatusFn st) {
    DownloadManager::EnsureDir(mcDir);
    DownloadManager::EnsureDir(joinPath(mcDir, "versions"));
    DownloadManager::EnsureDir(joinPath(mcDir, "libraries"));
    DownloadManager::EnsureDir(joinPath(mcDir, "assets"));
    DownloadManager::EnsureDir(joinPath(mcDir, "mods"));

    if (st) st("Fetching version manifest...", 0.02f);
    json manifest;
    if (!fetchManifest(manifest)) return false;

    std::string vjsonUrl;
    for (auto& v : manifest["versions"]) {
        if (v.value("id","") == "1.8.9") { vjsonUrl = v.value("url",""); break; }
    }
    if (vjsonUrl.empty()) return false;

    if (st) st("Fetching version metadata...", 0.05f);
    json vjson;
    if (!fetchVersionJson(vjsonUrl, vjson)) return false;

    // client.jar
    std::string clientUrl = vjson["downloads"]["client"].value("url","");
    std::string verDir = joinPath(mcDir, "versions/1.8.9");
    DownloadManager::EnsureDir(verDir);
    std::string clientDst = joinPath(verDir, "1.8.9.jar");
    if (!DownloadManager::FileExists(clientDst) && !clientUrl.empty()) {
        if (st) st("Downloading Minecraft client...", 0.10f);
        DownloadManager::Download(clientUrl, clientDst);
    }
    // save version json
    std::string vjsonDst = joinPath(verDir, "1.8.9.json");
    { std::ofstream o(vjsonDst); o << vjson.dump(); }

    downloadAllLibraries(mcDir, vjson, st, 0.15f, 0.55f);
    downloadAssets(mcDir, vjson, st, 0.70f, 0.98f);
    if (st) st("Ready", 1.f);
    return true;
}

// ---------- Forge install ----------
bool Minecraft::EnsureForge(const std::string& mcDir, const std::string& javaPath,
                            StatusFn st, std::string& outVersionId)
{
    // Forge 1.8.9-11.15.1.2318
    const std::string fv = "1.8.9-11.15.1.2318-1.8.9";
    outVersionId = "1.8.9-forge1.8.9-11.15.1.2318-1.8.9";

    std::string marker = joinPath(mcDir, "versions/" + outVersionId + "/" + outVersionId + ".json");
    if (DownloadManager::FileExists(marker)) return true;

    std::string instUrl =
        "https://maven.minecraftforge.net/net/minecraftforge/forge/"
        + fv + "/forge-" + fv + "-installer.jar";
    std::string instPath = joinPath(Settings::I().appDataDir(), "forge-installer.jar");

    if (st) st("Downloading Forge installer...", 0.30f);
    if (!DownloadManager::Download(instUrl, instPath)) return false;
    if (!DownloadManager::FileExists(instPath)) return false;

    if (st) st("Installing Forge (silent)...", 0.60f);

    // java -jar installer --installClient <mcDir>
    std::string cmd = "\"" + javaPath + "\" -jar \"" + instPath + "\" --installClient \"" + mcDir + "\"";
    STARTUPINFOA si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<char> cmdBuf(cmd.begin(), cmd.end()); cmdBuf.push_back(0);
    BOOL ok = CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (!ok) return false;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1; GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);

    if (st) st("Forge installed", 1.f);
    return DownloadManager::FileExists(marker);
}

// ---------- OptiFine ----------
bool Minecraft::EnsureOptiFine(const std::string& mcDir, StatusFn st) {
    std::string modsDir = joinPath(mcDir, "mods");
    DownloadManager::EnsureDir(modsDir);
    std::string dst = joinPath(modsDir, "OptiFine_1.8.9_HD_U_M5.jar");
    if (DownloadManager::FileExists(dst)) return true;

    if (st) st("Downloading OptiFine...", 0.50f);
    // Official OptiFine download mirror
    std::string url = "https://optifine.net/adloadx?f=OptiFine_1.8.9_HD_U_M5.jar";
    // Fallback: use BMCLAPI mirror
    std::string mirror = "https://bmclapi2.bangbang93.com/optifine/1.8.9/HD_U_M5";
    if (!DownloadManager::Download(mirror, dst)) {
        if (!DownloadManager::Download(url, dst)) return false;
    }
    return DownloadManager::FileExists(dst);
}

// ---------- Build classpath from version JSON ----------
static std::string buildClasspath(const std::string& mcDir, const json& vjson, const std::string& versionId) {
    std::vector<std::string> parts;
    if (vjson.contains("libraries")) {
        for (auto& lib : vjson["libraries"]) {
            if (lib.contains("rules")) {
                bool allow = false;
                for (auto& r : lib["rules"]) {
                    bool osMatch = true;
                    if (r.contains("os")) {
                        std::string osName = r["os"].value("name","");
                        if (!osName.empty() && osName != "windows") osMatch = false;
                    }
                    if (osMatch) allow = (r.value("action","allow") == "allow");
                }
                if (!allow) continue;
            }
            if (lib.contains("downloads") && lib["downloads"].contains("artifact")) {
                std::string rel = lib["downloads"]["artifact"].value("path","");
                if (!rel.empty()) parts.push_back(joinPath(mcDir, "libraries/" + rel));
            }
        }
    }
    // client.jar / forge jar
    std::string cj = joinPath(mcDir, "versions/" + versionId + "/" + versionId + ".jar");
    if (!DownloadManager::FileExists(cj))
        cj = joinPath(mcDir, "versions/1.8.9/1.8.9.jar");
    parts.push_back(cj);
    std::string cp;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) cp += ';';
        cp += parts[i];
    }
    return cp;
}

// ---------- Launch ----------
LaunchResult Minecraft::Launch(const std::string& mcDir,
                               const std::string& javaPath,
                               const std::string& versionId,
                               const std::string& username,
                               int ramMB,
                               int winW, int winH)
{
    LaunchResult r;
    std::string vjsonPath = joinPath(mcDir, "versions/" + versionId + "/" + versionId + ".json");
    std::ifstream f(vjsonPath);
    if (!f) { r.error = "Version JSON not found: " + vjsonPath; return r; }
    json vjson; try { f >> vjson; } catch (...) { r.error = "Invalid version JSON"; return r; }

    std::string mainClass = vjson.value("mainClass", "net.minecraft.client.main.Main");
    std::string cp = buildClasspath(mcDir, vjson, versionId);

    // Natives dir
    std::string natives = joinPath(mcDir, "versions/" + versionId + "/natives");
    DownloadManager::EnsureDir(natives);

    // Extract natives from all libs
    for (auto& lib : vjson["libraries"]) {
        if (lib.contains("natives")) {
            std::string cls = lib["natives"].value("windows","");
            if (!cls.empty() && lib["downloads"].contains("classifiers") &&
                lib["downloads"]["classifiers"].contains(cls))
            {
                std::string rel = lib["downloads"]["classifiers"][cls].value("path","");
                std::string jar = joinPath(mcDir, "libraries/" + rel);
                if (DownloadManager::FileExists(jar)) {
                    // extract jar entries ending in .dll
                    std::ifstream jf(jar, std::ios::binary);
                    if (jf) {
                        // Use miniz to unzip .dll files
                        // skip - too involved; use PowerShell Expand-Archive fallback
                        std::string ps =
                            "powershell -NoProfile -Command \"Add-Type -A System.IO.Compression.FileSystem;"
                            "[IO.Compression.ZipFile]::ExtractToDirectory('" + jar + "','" + natives + "')\"";
                        STARTUPINFOA si{}; si.cb = sizeof(si);
                        PROCESS_INFORMATION pi{};
                        std::vector<char> c(ps.begin(), ps.end()); c.push_back(0);
                        if (CreateProcessA(nullptr, c.data(), nullptr, nullptr, FALSE,
                                           CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                            WaitForSingleObject(pi.hProcess, 30000);
                            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
                        }
                    }
                }
            }
        }
    }

    // Assets
    std::string assetsDir = joinPath(mcDir, "assets");
    std::string assetIndex = vjson.value("assets", "1.8");
    std::string gameDir = mcDir;

    // Build launch command
    std::stringstream cmd;
    cmd << "\"" << javaPath << "\" ";
    cmd << "-Xmx" << ramMB << "M ";
    cmd << "-Djava.library.path=\"" << natives << "\" ";
    cmd << "-Dminecraft.launcher.brand=RavenXD -Dminecraft.launcher.version=1.0.0 ";
    cmd << "-cp \"" << cp << "\" ";
    cmd << mainClass << " ";
    cmd << "--username " << username << " ";
    cmd << "--version " << versionId << " ";
    cmd << "--gameDir \"" << gameDir << "\" ";
    cmd << "--assetsDir \"" << assetsDir << "\" ";
    cmd << "--assetIndex " << assetIndex << " ";
    cmd << "--uuid 00000000000000000000000000000000 ";
    cmd << "--accessToken 0 ";
    cmd << "--userType legacy ";
    cmd << "--versionType RavenXD ";
    cmd << "--width " << winW << " --height " << winH;

    std::string cmdline = cmd.str();

    STARTUPINFOA si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<char> cmdbuf(cmdline.begin(), cmdline.end()); cmdbuf.push_back(0);

    // Set working directory
    BOOL ok = CreateProcessA(nullptr, cmdbuf.data(), nullptr, nullptr, FALSE, 0,
                             nullptr, mcDir.c_str(), &si, &pi);
    if (!ok) { r.error = "Failed to start Minecraft"; return r; }
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    r.ok = true;
    return r;
}