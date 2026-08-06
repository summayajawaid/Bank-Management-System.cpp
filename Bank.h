#pragma once
/*
 * Bank.h
 * ------
 * The central service layer. Owns every account (via unique_ptr, so no
 * manual memory management or leaks), enforces authentication before any
 * sensitive operation, and coordinates persistence + audit logging around
 * every mutation. main.cpp (the UI layer) never touches accounts or files
 * directly -- it only calls Bank methods and reacts to exceptions.
 */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Account.h"
#include "FileManager.h"
#include "Logger.h"
#include "Transaction.h"

class Bank {
private:
    std::map<int, std::unique_ptr<Account>> accounts;
    int nextAccountNumber;
    long long nextTransactionId;
    FileManager fileManager;
    Logger logger;

    Account& getAccountOrThrow(int accountNumber);
    void recordTransaction(int accountNumber, TransactionType type, double amount,
                            double balanceAfter, const std::string& description);
    void persistAccounts() const;

public:
    explicit Bank(const std::string& dataDirectory = "data");

    void loadFromDisk();

    // --- Account lifecycle ---
    Account& createSavingsAccount(const std::string& name, const std::string& phone,
                                   const std::string& pin, double initialDeposit,
                                   double interestRate = 0.04, double minimumBalance = 500.0);
    Account& createCurrentAccount(const std::string& name, const std::string& phone,
                                   const std::string& pin, double initialDeposit,
                                   double overdraftLimit = 10000.0);
    void closeAccount(int accountNumber, const std::string& pin);

    // --- Authentication ---
    Account& authenticate(int accountNumber, const std::string& pin);

    // --- Transactions (all require a verified PIN except deposit) ---
    void deposit(int accountNumber, double amount);
    void withdraw(int accountNumber, double amount, const std::string& pin);
    void transfer(int fromAccount, int toAccount, double amount, const std::string& pin);
    void changePin(int accountNumber, const std::string& oldPin, const std::string& newPin);
    void updateContactInfo(int accountNumber, const std::string& pin,
                            const std::string& newName, const std::string& newPhone);

    // --- Reporting ---
    std::vector<Transaction> getStatement(int accountNumber) const;
    std::vector<const Account*> getAllAccounts() const;
    double getTotalAssets() const;
    size_t getActiveAccountCount() const;

    // --- Batch operations ---
    int applyMonthlyInterestToAllSavings();
};
