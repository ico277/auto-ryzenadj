#ifndef AUTORYZENADJ_UTIL_H
#define AUTORYZENADJ_UTIL_H

#include <iostream>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <stdexcept>
#include <vector>

class ThreadSafeLogger {
public:
    ThreadSafeLogger(const std::string& filename) {
        open(filename);
    }

    ThreadSafeLogger() {}

    void open(const std::string& filename) {
        std::lock_guard<std::mutex> lock(logger_mutex);
        filestream = std::ofstream(filename);
        if (!filestream->is_open()) {
            throw std::runtime_error("Failed to open log file");
        }
    }

    template <typename T> ThreadSafeLogger& operator<<(const T& message) {
        std::lock_guard<std::mutex> lock(logger_mutex);
        if (!filestream.has_value()) {
            std::cout << message;
            std::cout.flush();
        } else {
            *filestream << message;
            filestream->flush();
        }
        return *this;
    }

    ThreadSafeLogger& operator<<(std::ostream& (*manip)(std::ostream&)) {
        std::lock_guard<std::mutex> lock(logger_mutex);
        if (!filestream.has_value()) {
            manip(std::cout);
            std::cout.flush();
        } else {
            manip(*filestream);
            filestream->flush();
        }
        return *this;
    }

private:
    std::optional<std::ofstream> filestream = std::nullopt;
    std::mutex logger_mutex;
};

struct Config {
    std::map<std::string, std::vector<std::string>> profiles;
    long timer = 0;
    std::string cur_profile;
    std::string executable;
    std::string socket_group;
    std::mutex mutex;
};

inline std::string replaceAll(std::string str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

#endif
