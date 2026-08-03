#include "CurrentAccount.h"
#include "Exceptions.h"
#include "Utils.h"

#include <sstream>
#include <iomanip>

CurrentAccount::CurrentAccount(int accountNumber, const std::string& holderName, const std::string& phoneNumber,
                                const std::string& pinHash, const std::string& pinSalt, double initialBalance,
                                double overdraftLimit_)
    : Account(accountNumber, holderName, phoneNumber, pinHash, pinSalt, initialBalance),
      overdraftLimit(overdraftLimit_) {}

double CurrentAccount::getWithdrawableLimit() const {
    return balance + overdraftLimit;
}

void CurrentAccount::withdraw(double amount) {
    if (!bankutils::isValidAmount(amount)) {
        throw InvalidAmountException("withdrawal amount must be positive");
    }
    if (amount > getWithdrawableLimit()) {
        throw InsufficientFundsException(amount, getWithdrawableLimit());
    }
    balance -= amount; // may legally go negative, down to -overdraftLimit
}

std::string CurrentAccount::extraFieldsToFileLine() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << overdraftLimit << "|0";
    return oss.str();
}
