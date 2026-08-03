#include "FileManager.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

FileManager::FileManager(const std::string& dataDirectory_)
    : dataDirectory(dataDirectory_),
      accountsFilePath(dataDirectory_ + "/accounts.dat"),
      transactionsFilePath(dataDirectory_ + "/transactions.dat") {
    ensureDataDirectoryExists();
}

void FileManager::ensureDataDirectoryExists() const {
    if (!fs::exists(dataDirectory)) {
        fs::create_directories(dataDirectory);
    }
}

std::vector<std::string> FileManager::readAccountLines() const {
    std::vector<std::string> lines;
    std::ifstream fin(accountsFilePath);
    if (!fin.is_open()) return lines; // no file yet == no accounts yet

    std::string line;
    while (std::getline(fin, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

void FileManager::writeAccountLines(const std::vector<std::string>& lines) const {
    std::ofstream fout(accountsFilePath, std::ios::trunc);
    if (!fout.is_open()) {
        throw std::runtime_error("Unable to open " + accountsFilePath + " for writing");
    }
    for (const auto& line : lines) {
        fout << line << "\n";
    }
}

void FileManager::appendTransactionLine(const std::string& line) const {
    std::ofstream fout(transactionsFilePath, std::ios::app);
    if (!fout.is_open()) {
        throw std::runtime_error("Unable to open " + transactionsFilePath + " for writing");
    }
    fout << line << "\n";
}

std::vector<std::string> FileManager::readAllTransactionLines() const {
    std::vector<std::string> lines;
    std::ifstream fin(transactionsFilePath);
    if (!fin.is_open()) return lines;

    std::string line;
    while (std::getline(fin, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}
