//
// Created by xyz on 22-5-26.
//

#ifndef MULTI_GET_LOGGER_H
#define MULTI_GET_LOGGER_H

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

static std::string getTime() {
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    static char tmp[32] = {};
    tm stime{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&stime, &now);
#elif defined(__linux__)
    localtime_r(&now, &stime);
#endif
    std::strftime(tmp, sizeof(tmp), "%Y.%m.%d %H:%M:%S", &stime);
    return {tmp};
}

// ============================================================
// Here is an example of a simple log decorator, you can define your own decorator
// ============================================================
class TextDecorator {
  public:
    static std::string FileHeader(const std::string &p_title) {
        return "==================================================\n" +
               p_title + "\n" +
               "==================================================\n\n";
    }

    static std::string SessionOpen() {
        return "\n";
    }

    static std::string SessionClose() {
        return "\n";
    }

    static std::string Decorate(const std::string &p_string) {
        return p_string + "\n";
    }
};

// ============================================================
// New Logger with a new log file and new log title
// ============================================================
template <class decorator>
class Logger {
  public:
    static Logger &getInstance() {
        static Logger logger;
        return logger;
    }
    Logger &setLogFile(const std::string &filename) {
        if (log_file_opened) {
            m_logfile.flush();
            m_logfile.close();
        }
        if (!std::filesystem::exists(filename)) {
            m_logfile.open(filename, std::ios::out | std::ios::app);
            if (file_header.empty())
                m_logfile << decorator::FileHeader(filename);
            else
                m_logfile << decorator::FileHeader(file_header);
        } else {
            m_logfile.open(filename, std::ios::out | std::ios::app);
        }
        log_file_opened = true;
        m_logfile << decorator::SessionOpen();
        Log("%s", "Session opened.");
        return *this;
    }

    Logger &setTimeStamp(bool on) {
        m_timestamp = on;
        return *this;
    }

    Logger &setFormat(const std::string &fmt) {
        format_header = fmt;
        return *this;
    }

    Logger &setFileHeader(const std::string &header) {
        file_header = header;
        return *this;
    }

    template <class T>
    Logger &operator<<(const T &lhs) {
        Log(lhs);
        return *this;
    }

    ~Logger() {
        if (log_file_opened) {
            Log("%s", "Session closed.");
            m_logfile << decorator::SessionClose();
            m_logfile.flush();
            m_logfile.close();
        }
    }

    template <class... Args>
    void Log(const char *const fmt, const Args &...args) {
        std::unique_lock<std::mutex> locker(mtx);
        std::string line_header{};
        if (m_timestamp) {
            size_t len = std::snprintf(m_buffer, BUF_LEN, format_header.c_str(), getTime().c_str());
            line_header = std::string{m_buffer, len};
        }

        if constexpr (sizeof...(args) < 1) {
            m_logfile << decorator::Decorate(line_header + std::string(fmt));
        } else {
            size_t len = std::snprintf(m_buffer, BUF_LEN, fmt, args...);
            std::string content(m_buffer, len);
            m_logfile << decorator::Decorate(line_header + content);
        }
        m_logfile.flush();
    }

    void Log(const std::string &s) {
        Log(s.c_str());
    }

  protected:
    std::ofstream m_logfile;
    std::string m_log_filename, format_header{"[%s] "}, file_header;
    bool m_timestamp{true};
    bool log_file_opened{false};
    static constexpr size_t BUF_LEN = 1024;
    char m_buffer[BUF_LEN]{0};
    std::mutex mtx;

    Logger() = default;
};

using TextLog = Logger<TextDecorator>;

#define LOGGER TextLog::getInstance()

#define LOG_INFO(fmt, ...)                                                                                                                                                       \
    do {                                                                                                                                                                             \
        char _buf[1024] = {0};                                                                                                                                                       \
        snprintf(_buf, sizeof(_buf), "[ INFO][%20s:%20s:%4d] %s", std::string(__FILE__).substr(std::string(__FILE__).find_last_of("/\\") + 1).c_str(), __FUNCTION__, __LINE__, fmt); \
        TextLog::getInstance().Log(_buf, ##__VA_ARGS__);                                                                                                                                    \
    } while (false)


//"[ WARN][%20s:%20s:%4d] %s"
#define LOG_WARN(fmt, ...)                                                                                                                                                       \
    do {                                                                                                                                                                             \
        char _buf[1024] = {0};                                                                                                                                                       \
        snprintf(_buf, sizeof(_buf), "[ WARN][%20s:%20s:%4d] %s", std::string(__FILE__).substr(std::string(__FILE__).find_last_of("/\\") + 1).c_str(), __FUNCTION__, __LINE__, fmt); \
        TextLog::getInstance().Log(_buf, ##__VA_ARGS__);                                                                                                                                    \
    } while (false)

#define LOG_ERROR(fmt, ...)                                                                                                                                                      \
    do {                                                                                                                                                                             \
        char _buf[1024] = {0};                                                                                                                                                       \
        snprintf(_buf, sizeof(_buf), "[ERROR][%20s:%20s:%4d] %s", std::string(__FILE__).substr(std::string(__FILE__).find_last_of("/\\") + 1).c_str(), __FUNCTION__, __LINE__, fmt); \
        TextLog::getInstance().Log(_buf, ##__VA_ARGS__);                                                                                                                                    \
    } while (false)

#endif // MULTI_GET_LOGGER_H
