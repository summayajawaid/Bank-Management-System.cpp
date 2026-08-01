#include "Account.h"
#include "Exceptions.h"
#include "Utils.h"

#include <iostream>
#include <iomanip>
#include <sstream>

Account::Account(int accountNumber_, std::string holderName_, std::string phoneNumber_,
                  std::string pinHash_, std::string pinSalt_, double initialBalance)
    : accountNumber(accountNumber_), holderName(std::move(holderName_)),
      phoneNumber(std::move(phoneNumber_)), pinHash(std::move(pinHash_)),
      pinSalt(std::move(pinSalt_)), balance(initialBalance), active(true) {}

void Account::deposit(double amount) {
    if (!bankutils::isValidAmount(amount)) {
        throw InvalidAmountException("deposit amount must be positive");
    }
    balance += amount;
}

void Account::withdraw(double amount) {
    if (!bankutils::isValidAmount(amount)) {
        throw InvalidAmountException("withdrawal amount must be positive");
    }
    if (amount > getWithdrawableLimit()) {
        throw InsufficientFundsException(amount, getWithdrawableLimit());
    }
    balance -= amount;
}

double Account::getWithdrawableLimit() const {
    return balance;
}

bool Account::verifyPin(const std::string& pin) const {
    return bankutils::hashPin(pin, pinSalt) == pinHash;
}

void Account::changePin(const std::string& newPin) {
    if (!bankutils::isValidPin(newPin)) {
        throw ValidationException("PIN must be 4-6 digits");
    }
    pinSalt = bankutils::generateSalt();
    pinHash = bankutils::hashPin(newPin, pinSalt);
}

std::string Account::commonFieldsToFileLine() const {
    std::ostringstream oss;
    oss << accountNumber << "|" << holderName << "|" << phoneNumber << "|"
        << pinHash << "|" << pinSalt << "|"
        << std::fixed << std::setprecision(2) << balance << "|"
        << (active ? "1" : "0");
    return oss.str();
}

void Account::display() const {
    std::cout << "\n----------------------------------------\n";
    std::cout << std::left << std::setw(20) << "Account Number:" << accountNumber << "\n";
    std::cout << std::left << std::setw(20) << "Account Type:"   << getAccountType() << "\n";
    std::cout << std::left << std::setw(20) << "Holder Name:"    << holderName << "\n";
    std::cout << std::left << std::setw(20) << "Phone Number:"   << phoneNumber << "\n";
    std::cout << std::left << std::setw(20) << "Status:"         << (active ? "Active" : "Closed") << "\n";
    std::cout << std::left << std::setw(20) << "Balance:"        << std::fixed << std::setprecision(2) << balance << "\n";
    std::cout << "----------------------------------------\n";
}
