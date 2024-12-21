#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <regex>
#include <cstdlib>

namespace fs = std::filesystem;

struct CleanupItem {
    std::string path;
    bool isFile = false;
    bool isFilePattern = false;
    bool recursive = false;
};

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm;
    gmtime_s(&utc_tm, &now_time);
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y%m%d%H%M%S");
    return oss.str();
}

std::string getEnvVar(const std::string& key) {
    char* val = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&val, &len, key.c_str());
    if (err || val == nullptr) {
        return "";
    }
    std::string value(val);
    free(val);
    return value;
}

int main() {
    std::vector<CleanupItem> cleanupItems = {
        { "%systemdrive%\\Users\\%username%\\AppData\\LocalLow\\rbxcsettings.rbx", true, false, false },
        { "%systemdrive%\\Users\\%username%\\AppData\\Local\\Roblox\\GlobalBasicSettings_13.xml", true, false, false },
        { "%systemdrive%\\Users\\%username%\\AppData\\Local\\Roblox\\RobloxCookies.dat", true, false, false },
        { "%systemdrive%\\Users\\%username%\\AppData\\Local\\Roblox\\frm.cfg", true, false, false },
        { "%systemdrive%\\Users\\%username%\\AppData\\Local\\Roblox\\AnalysticsSettings.xml", true, false, false },
        { "%systemdrive%\\Users\\%username%\\AppData\\Local\\Roblox\\LocalStorage\\*", false, false, true },
        { "%systemdrive%\\Users\\%username%\\AppData\\Local\\Roblox\\logs\\*", false, false, true },
        { "%temp%\\RBX-*.log", true, true, false }, 
        { "%systemdrive%\\Windows\\Temp\\*", false, false, true },
        { "%systemdrive%\\Users\\%username%\\AppData\\Local\\Microsoft\\CLR_v4.0_32\\UsageLogs\\*", false, false, true },
        { "%systemdrive%\\Users\\%username%\\AppData\\Local\\Microsoft\\CLR_v4.0\\UsageLogs\\*", false, false, true }
    };

    for (auto& item : cleanupItems) {
        size_t pos = 0;
        while ((pos = item.path.find('%', pos)) != std::string::npos) {
            size_t end = item.path.find('%', pos + 1);
            if (end == std::string::npos) break;
            std::string var = item.path.substr(pos + 1, end - pos - 1);
            std::string val = getEnvVar(var);
            item.path.replace(pos, end - pos + 1, val);
            pos += val.length();
        }
    }

    std::cout << "[LOG][" << getCurrentTimestamp() << "] Cleaning started\n";

    for (const auto& item : cleanupItems) {
        try {
            if (item.isFilePattern) {
                fs::path p(item.path);
                fs::path dir = p.parent_path();
                std::string pattern = p.filename().string();
                if (fs::exists(dir) && fs::is_directory(dir)) {
                    std::regex regexPattern("^" + std::regex_replace(pattern, std::regex(R"(\*)"), ".*") + "$");
                    for (const auto& entry : fs::directory_iterator(dir)) {
                        if (std::regex_match(entry.path().filename().string(), regexPattern)) {
                            fs::remove(entry.path());
                            std::cout << "[LOG][" << getCurrentTimestamp() << "] Deleted file: " << entry.path() << "\n";
                        }
                    }
                }
            }
            else if (item.isFile) {
                fs::path p(item.path);
                if (fs::exists(p) && fs::is_regular_file(p)) {
                    fs::remove(p);
                    std::cout << "[LOG][" << getCurrentTimestamp() << "] Deleted file: " << p << "\n";
                }
            }
            else {
                fs::path p(item.path);
                if (fs::exists(p) && fs::is_directory(p)) {
                    for (const auto& entry : fs::recursive_directory_iterator(p)) {
                        if (fs::is_regular_file(entry.path())) {
                            fs::remove(entry.path());
                            std::cout << "[LOG][" << getCurrentTimestamp() << "] Deleted file: " << entry.path() << "\n";
                        }
                    }
                    if (item.recursive) {
                        fs::remove_all(p);
                        std::cout << "[LOG][" << getCurrentTimestamp() << "] Deleted directory: " << p << "\n";
                    }
                }
            }
        }
        catch (const std::exception& ex) {
            std::cout << "[EXCEPTION][" << getCurrentTimestamp() << "] Error cleaning '" << item.path << "': " << ex.what() << "\n";
        }
    }
    std::cout << "[LOG][" << getCurrentTimestamp() << "] Cleaning ended\n";
    system("pause");
    return 0;
}