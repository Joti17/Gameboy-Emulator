#pragma once

#include <fstream>
#include <format>
#include <string>
#include <cstdlib>
#include <filesystem>


struct Logger {
    std::fstream logFile;

    static std::filesystem::path default_log_path() {
        namespace fs = std::filesystem;

#ifdef _WIN32
        const char* base = std::getenv("LOCALAPPDATA");
        fs::path path = base ? base : ".";
        path /= "timme_dmg/log.txt";

#elif __APPLE__
        const char* home = std::getenv("HOME");
        fs::path path = home ? home : ".";
        path /= "Library/Application Support/timme_dmg/log.txt";

#else
        const char* home = std::getenv("HOME");
        fs::path path = home ? home : ".";
        path /= ".local/share/timme_dmg/log.txt";
#endif

        return path;
    }

    Logger(const std::string& filename = "")
    {
        namespace fs = std::filesystem;

        fs::path path = filename.empty()
            ? default_log_path()
            : fs::path(filename);

        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);

        logFile.open(path, std::ios::out | std::ios::app);
    }

    template<typename... Args>
    void log(const std::string& fmt, Args&&... args) {
        if (!logFile.is_open())
            return;

        logFile << std::vformat(fmt, std::make_format_args(args...)) << '\n';
    }
};


extern Logger g_logger;