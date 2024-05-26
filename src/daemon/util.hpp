#ifndef AUTORYZENADJ_UTIL_H
#define AUTORYZENADJ_UTIL_H

#include <iostream>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <stdexcept>
#include <vector>

class ThreadSafeLogger {
public:
    ThreadSafeLogger(const std::string& filename) : logger(filename) {
        if (!logger.is_open()) {
            throw std::runtime_error("Failed to open log file.");
        }
    }
    ThreadSafeLogger() : logger("/dev/stdout") {
        if (!logger.is_open()) {
            throw std::runtime_error("Failed to open /dev/stdout.");
        }
    }

    template <typename T> ThreadSafeLogger& operator<<(const T& message) {
        std::lock_guard<std::mutex> lock(logger_mutex);
        logger << message;
        logger.flush();
        return *this;
    }

    ThreadSafeLogger& operator<<(std::ostream& (*manip)(std::ostream&)) {
        std::lock_guard<std::mutex> lock(logger_mutex);
        manip(logger);
        logger.flush();
        return *this;
    }

private:
    std::ofstream logger;
    std::mutex logger_mutex;
};

struct Config {
    std::map<std::string, std::vector<std::string>> profiles;
    long timer = 0;
    std::string cur_profile;
    std::string executable;
    std::mutex mutex;
};

#endif
