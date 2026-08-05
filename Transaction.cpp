#include "Transaction.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

std::string transactionTypeToString(TransactionType type) {
    switch (type) {
        case TransactionType::DEPOSIT:        return "DEPOSIT";
        case TransactionType::WITHDRAWAL:     return "WITHDRAWAL";
        case TransactionType::TRANSFER_IN:    return "TRANSFER_IN";
        case TransactionType::TRANSFER_OUT:   return "TRANSFER_OUT";
        case TransactionType::INTEREST:       return "INTEREST";
        case TransactionType::ACCOUNT_OPENED: return "ACCOUNT_OPENED";
        case TransactionType::ACCOUNT_CLOSED: return "ACCOUNT_CLOSED";
    }
    return "UNKNOWN";
}

TransactionType transactionTypeFromString(const std::string& text) {
    if (text == "DEPOSIT")        return TransactionType::DEPOSIT;
    if (text == "WITHDRAWAL")     return TransactionType::WITHDRAWAL;
    if (text == "TRANSFER_IN")    return TransactionType::TRANSFER_IN;
    if (text == "TRANSFER_OUT")   return TransactionType::TRANSFER_OUT;
    if (text == "INTEREST")       return TransactionType::INTEREST;
    if (text == "ACCOUNT_OPENED") return TransactionType::ACCOUNT_OPENED;
    if (text == "ACCOUNT_CLOSED") return TransactionType::ACCOUNT_CLOSED;
    throw std::runtime_error("Unknown transaction type in file: " + text);
}

Transaction::Transaction(long long id_, int accountNumber_, TransactionType type_, double amount_,
                          double balanceAfter_, std::string timestamp_, std::string description_)
    : id(id_), accountNumber(accountNumber_), type(type_), amount(amount_),
      balanceAfter(balanceAfter_), timestamp(std::move(timestamp_)), description(std::move(description_)) {}

std::string Transaction::toFileLine() const {
    std::ostringstream oss;
    oss << id << "|" << accountNumber << "|" << transactionTypeToString(type) << "|"
        << std::fixed << std::setprecision(2) << amount << "|" << balanceAfter << "|"
        << timestamp << "|" << description;
    return oss.str();
}

Transaction Transaction::fromFileLine(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    Transaction t;

    std::getline(iss, token, '|'); t.id = std::stoll(token);
    std::getline(iss, token, '|'); t.accountNumber = std::stoi(token);
    std::getline(iss, token, '|'); t.type = transactionTypeFromString(token);
    std::getline(iss, token, '|'); t.amount = std::stod(token);
    std::getline(iss, token, '|'); t.balanceAfter = std::stod(token);
    std::getline(iss, token, '|'); t.timestamp = token;
    std::getline(iss, t.description); // remainder of line (description may be empty)

    return t;
}

void Transaction::display() const {
    std::cout << std::left
               << std::setw(21) << timestamp
               << std::setw(14) << transactionTypeToString(type)
               << std::right << std::setw(12) << std::fixed << std::setprecision(2) << amount
               << std::setw(14) << balanceAfter << "   " << description << "\n";
}
