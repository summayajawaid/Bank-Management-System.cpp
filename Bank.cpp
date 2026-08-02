#include "Bank.h"
#include "SavingsAccount.h"
#include "CurrentAccount.h"
#include "Exceptions.h"
#include "Utils.h"

#include <algorithm>
#include <sstream>
#include <iostream>

Bank::Bank(const std::string& dataDirectory)
    : nextAccountNumber(1001), nextTransactionId(1),
      fileManager(dataDirectory), logger(dataDirectory + "/audit.log") {
    loadFromDisk();
}

// ---------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------

void Bank::loadFromDisk() {
    accounts.clear();
    int maxAccountNumber = 1000;

    for (const auto& line : fileManager.readAccountLines()) {
        // Format: type|accNo|name|phone|pinHash|pinSalt|balance|active|extra1|extra2
        std::istringstream iss(line);
        std::string typeToken, accNoToken, name, phone, pinHash, pinSalt, balanceToken, activeToken, extra1, extra2;

        std::getline(iss, typeToken, '|');
        std::getline(iss, accNoToken, '|');
        std::getline(iss, name, '|');
        std::getline(iss, phone, '|');
        std::getline(iss, pinHash, '|');
        std::getline(iss, pinSalt, '|');
        std::getline(iss, balanceToken, '|');
        std::getline(iss, activeToken, '|');
        std::getline(iss, extra1, '|');
        std::getline(iss, extra2, '|');

        int accNo = std::stoi(accNoToken);
        double balance = std::stod(balanceToken);
        bool active = (activeToken == "1");

        std::unique_ptr<Account> acc;
        if (typeToken == "S") {
            double rate = extra1.empty() ? 0.04 : std::stod(extra1);
            double minBal = extra2.empty() ? 500.0 : std::stod(extra2);
            acc = std::make_unique<SavingsAccount>(accNo, name, phone, pinHash, pinSalt, balance, rate, minBal);
        } else if (typeToken == "C") {
            double overdraft = extra1.empty() ? 10000.0 : std::stod(extra1);
            acc = std::make_unique<CurrentAccount>(accNo, name, phone, pinHash, pinSalt, balance, overdraft);
        } else {
            continue; // skip malformed/unknown rows rather than crashing on load
        }

        if (!active) acc->deactivate();
        maxAccountNumber = std::max(maxAccountNumber, accNo);
        accounts[accNo] = std::move(acc);
    }

    nextAccountNumber = maxAccountNumber + 1;

    // Resume transaction IDs after the highest one seen on disk.
    long long maxTxId = 0;
    for (const auto& line : fileManager.readAllTransactionLines()) {
        auto pos = line.find('|');
        if (pos != std::string::npos) {
            try {
                maxTxId = std::max(maxTxId, std::stoll(line.substr(0, pos)));
            } catch (...) { /* ignore malformed line */ }
        }
    }
    nextTransactionId = maxTxId + 1;
}

void Bank::persistAccounts() const {
    std::vector<std::string> lines;
    lines.reserve(accounts.size());
    for (const auto& [accNo, acc] : accounts) {
        char typeChar = (acc->getAccountType() == "Savings") ? 'S' : 'C';
        lines.push_back(std::string(1, typeChar) + "|" + acc->commonFieldsToFileLine() + "|" + acc->extraFieldsToFileLine());
    }
    fileManager.writeAccountLines(lines);
}

void Bank::recordTransaction(int accountNumber, TransactionType type, double amount,
                              double balanceAfter, const std::string& description) {
    Transaction t(nextTransactionId++, accountNumber, type, amount, balanceAfter,
                  bankutils::currentTimestamp(), description);
    fileManager.appendTransactionLine(t.toFileLine());
}

// ---------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------

Account& Bank::getAccountOrThrow(int accountNumber) {
    auto it = accounts.find(accountNumber);
    if (it == accounts.end()) {
        throw AccountNotFoundException(accountNumber);
    }
    if (!it->second->isActive()) {
        throw AccountInactiveException(accountNumber);
    }
    return *it->second;
}

// ---------------------------------------------------------------------
// Account lifecycle
// ---------------------------------------------------------------------

Account& Bank::createSavingsAccount(const std::string& name, const std::string& phone,
                                     const std::string& pin, double initialDeposit,
                                     double interestRate, double minimumBalance) {
    if (!bankutils::isValidName(name)) throw ValidationException("holder name cannot be empty");
    if (!bankutils::isValidPin(pin)) throw ValidationException("PIN must be 4-6 digits");
    if (initialDeposit < 0) throw InvalidAmountException("initial deposit cannot be negative");

    int accNo = nextAccountNumber++;
    std::string salt = bankutils::generateSalt();
    std::string hash = bankutils::hashPin(pin, salt);

    accounts[accNo] = std::make_unique<SavingsAccount>(accNo, name, phone, hash, salt,
                                                         initialDeposit, interestRate, minimumBalance);
    persistAccounts();
    recordTransaction(accNo, TransactionType::ACCOUNT_OPENED, initialDeposit, initialDeposit, "Savings account opened");
    logger.log("Created Savings account #" + std::to_string(accNo) + " for " + name);

    return *accounts[accNo];
}

Account& Bank::createCurrentAccount(const std::string& name, const std::string& phone,
                                     const std::string& pin, double initialDeposit,
                                     double overdraftLimit) {
    if (!bankutils::isValidName(name)) throw ValidationException("holder name cannot be empty");
    if (!bankutils::isValidPin(pin)) throw ValidationException("PIN must be 4-6 digits");
    if (initialDeposit < 0) throw InvalidAmountException("initial deposit cannot be negative");

    int accNo = nextAccountNumber++;
    std::string salt = bankutils::generateSalt();
    std::string hash = bankutils::hashPin(pin, salt);

    accounts[accNo] = std::make_unique<CurrentAccount>(accNo, name, phone, hash, salt,
                                                         initialDeposit, overdraftLimit);
    persistAccounts();
    recordTransaction(accNo, TransactionType::ACCOUNT_OPENED, initialDeposit, initialDeposit, "Current account opened");
    logger.log("Created Current account #" + std::to_string(accNo) + " for " + name);

    return *accounts[accNo];
}

void Bank::closeAccount(int accountNumber, const std::string& pin) {
    Account& acc = getAccountOrThrow(accountNumber);
    if (!acc.verifyPin(pin)) {
        logger.log("Failed close attempt on account #" + std::to_string(accountNumber) + ": bad PIN");
        throw AuthenticationException();
    }
    acc.deactivate();
    persistAccounts();
    recordTransaction(accountNumber, TransactionType::ACCOUNT_CLOSED, 0.0, acc.getBalance(), "Account closed");
    logger.log("Closed account #" + std::to_string(accountNumber));
}

// ---------------------------------------------------------------------
// Authentication
// ---------------------------------------------------------------------

Account& Bank::authenticate(int accountNumber, const std::string& pin) {
    Account& acc = getAccountOrThrow(accountNumber);
    if (!acc.verifyPin(pin)) {
        logger.log("Failed login attempt on account #" + std::to_string(accountNumber));
        throw AuthenticationException();
    }
    logger.log("Successful login on account #" + std::to_string(accountNumber));
    return acc;
}

// ---------------------------------------------------------------------
// Transactions
// ---------------------------------------------------------------------

void Bank::deposit(int accountNumber, double amount) {
    Account& acc = getAccountOrThrow(accountNumber);
    acc.deposit(amount);
    persistAccounts();
    recordTransaction(accountNumber, TransactionType::DEPOSIT, amount, acc.getBalance(), "Cash deposit");
}

void Bank::withdraw(int accountNumber, double amount, const std::string& pin) {
    Account& acc = getAccountOrThrow(accountNumber);
    if (!acc.verifyPin(pin)) {
        logger.log("Failed withdrawal attempt on account #" + std::to_string(accountNumber) + ": bad PIN");
        throw AuthenticationException();
    }
    acc.withdraw(amount);
    persistAccounts();
    recordTransaction(accountNumber, TransactionType::WITHDRAWAL, amount, acc.getBalance(), "Cash withdrawal");
}

void Bank::transfer(int fromAccount, int toAccount, double amount, const std::string& pin) {
    if (fromAccount == toAccount) {
        throw ValidationException("source and destination accounts must differ");
    }

    Account& src = getAccountOrThrow(fromAccount);
    Account& dst = getAccountOrThrow(toAccount); // ensures destination exists & active before touching balances

    if (!src.verifyPin(pin)) {
        logger.log("Failed transfer attempt from account #" + std::to_string(fromAccount) + ": bad PIN");
        throw AuthenticationException();
    }

    // Withdraw first; only deposit if that succeeds, so a failure never
    // leaves money debited from one account without crediting the other.
    src.withdraw(amount);
    try {
        dst.deposit(amount);
    } catch (...) {
        src.deposit(amount); // roll back the debit
        throw;
    }

    persistAccounts();
    recordTransaction(fromAccount, TransactionType::TRANSFER_OUT, amount, src.getBalance(),
                       "Transfer to account #" + std::to_string(toAccount));
    recordTransaction(toAccount, TransactionType::TRANSFER_IN, amount, dst.getBalance(),
                       "Transfer from account #" + std::to_string(fromAccount));
}

void Bank::changePin(int accountNumber, const std::string& oldPin, const std::string& newPin) {
    Account& acc = getAccountOrThrow(accountNumber);
    if (!acc.verifyPin(oldPin)) {
        throw AuthenticationException();
    }
    acc.changePin(newPin);
    persistAccounts();
    logger.log("PIN changed for account #" + std::to_string(accountNumber));
}

void Bank::updateContactInfo(int accountNumber, const std::string& pin,
                              const std::string& newName, const std::string& newPhone) {
    Account& acc = getAccountOrThrow(accountNumber);
    if (!acc.verifyPin(pin)) {
        throw AuthenticationException();
    }
    if (!newName.empty()) acc.setHolderName(newName);
    if (!newPhone.empty()) acc.setPhoneNumber(newPhone);
    persistAccounts();
    logger.log("Contact info updated for account #" + std::to_string(accountNumber));
}

// ---------------------------------------------------------------------
// Reporting
// ---------------------------------------------------------------------

std::vector<Transaction> Bank::getStatement(int accountNumber) const {
    std::vector<Transaction> statement;
    for (const auto& line : fileManager.readAllTransactionLines()) {
        Transaction t = Transaction::fromFileLine(line);
        if (t.accountNumber == accountNumber) statement.push_back(t);
    }
    return statement;
}

std::vector<const Account*> Bank::getAllAccounts() const {
    std::vector<const Account*> result;
    result.reserve(accounts.size());
    for (const auto& [accNo, acc] : accounts) {
        result.push_back(acc.get());
    }
    return result;
}

double Bank::getTotalAssets() const {
    double total = 0.0;
    for (const auto& [accNo, acc] : accounts) {
        if (acc->isActive()) total += acc->getBalance();
    }
    return total;
}

size_t Bank::getActiveAccountCount() const {
    return std::count_if(accounts.begin(), accounts.end(),
                          [](const auto& pair) { return pair.second->isActive(); });
}

// ---------------------------------------------------------------------
// Batch operations
// ---------------------------------------------------------------------

int Bank::applyMonthlyInterestToAllSavings() {
    int count = 0;
    for (auto& [accNo, acc] : accounts) {
        if (acc->isActive() && acc->getAccountType() == "Savings") {
            double before = acc->getBalance();
            acc->applyPeriodicAdjustment();
            double credited = acc->getBalance() - before;
            if (credited > 0.0) {
                recordTransaction(accNo, TransactionType::INTEREST, credited, acc->getBalance(), "Monthly interest credit");
                ++count;
            }
        }
    }
    persistAccounts();
    logger.log("Applied monthly interest to " + std::to_string(count) + " savings account(s)");
    return count;
}
