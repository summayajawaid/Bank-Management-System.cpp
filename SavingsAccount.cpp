#include "SavingsAccount.h"
#include "Exceptions.h"
#include "Utils.h"

#include <sstream>
#include <iomanip>

SavingsAccount::SavingsAccount(int accountNumber, const std::string& holderName, const std::string& phoneNumber,
                                const std::string& pinHash, const std::string& pinSalt, double initialBalance,
                                double interestRate_, double minimumBalance_)
    : Account(accountNumber, holderName, phoneNumber, pinHash, pinSalt, initialBalance),
      interestRate(interestRate_), minimumBalance(minimumBalance_) {}

double SavingsAccount::getWithdrawableLimit() const {
    double limit = balance - minimumBalance;
    return limit > 0.0 ? limit : 0.0;
}

void SavingsAccount::withdraw(double amount) {
    if (!bankutils::isValidAmount(amount)) {
        throw InvalidAmountException("withdrawal amount must be positive");
    }
    if (amount > getWithdrawableLimit()) {
        throw InsufficientFundsException(amount, getWithdrawableLimit());
    }
    balance -= amount;
}

void SavingsAccount::applyPeriodicAdjustment() {
    // Simple monthly credit: annual rate / 12, applied to current balance.
    double interest = balance * (interestRate / 12.0);
    if (interest > 0.0) {
        balance += interest;
    }
}

std::string SavingsAccount::extraFieldsToFileLine() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4) << interestRate << "|" << std::setprecision(2) << minimumBalance;
    return oss.str();
}
