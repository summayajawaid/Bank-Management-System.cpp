#pragma once
/*
 * CurrentAccount.h
 * ----------------
 * A checking-style account that permits the balance to go negative up to
 * an overdraft limit. Pays no interest.
 */

#include "Account.h"

class CurrentAccount : public Account {
private:
    double overdraftLimit;

public:
    CurrentAccount(int accountNumber, const std::string& holderName, const std::string& phoneNumber,
                   const std::string& pinHash, const std::string& pinSalt, double initialBalance,
                   double overdraftLimit = 10000.0);

    std::string getAccountType() const override { return "Current"; }
    void withdraw(double amount) override;
    double getWithdrawableLimit() const override;
    std::string extraFieldsToFileLine() const override;

    double getOverdraftLimit() const { return overdraftLimit; }
};
