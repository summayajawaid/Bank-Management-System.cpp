#include "Logger.h"
#include "Utils.h"

#include <fstream>
#include <iostream>

Logger::Logger(const std::string& logFilePath_) : logFilePath(logFilePath_) {}

void Logger::log(const std::string& message) const {
    std::ofstream fout(logFilePath, std::ios::app);
    if (!fout.is_open()) {
        std::cerr << "[Logger] Warning: could not open " << logFilePath << "\n";
        return;
    }
    fout << "[" << bankutils::currentTimestamp() << "] " << message << "\n";
}
