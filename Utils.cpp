#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <functional>
#include <random>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace bankutils {

std::string trim(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &t);
#else
    localtime_r(&t, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string generateSalt(size_t length) {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, sizeof(alphabet) - 2);

    std::string salt;
    salt.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        salt += alphabet[dist(rng)];
    }
    return salt;
}

std::string hashPin(const std::string& pin, const std::string& salt) {
    // Combine PIN + salt and run through std::hash, then render as hex.
    // See the security note in Utils.h.
    std::hash<std::string> hasher;
    size_t h = hasher(salt + ":" + pin + ":" + salt);

    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << h;
    return oss.str();
}

bool isAllDigits(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

bool isValidAmount(double amount) {
    return std::isfinite(amount) && amount > 0.0;
}

bool isValidPin(const std::string& pin) {
    return isAllDigits(pin) && pin.size() >= 4 && pin.size() <= 6;
}

bool isValidName(const std::string& name) {
    return !trim(name).empty();
}

} // namespace bankutils
