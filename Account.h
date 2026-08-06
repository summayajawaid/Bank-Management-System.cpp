#pragma once
/*
 * Account.h
 * ---------
 * Abstract base class for all account types. Encapsulates the invariants
 * every account shares (balance can't go negative arbitrarily, PIN
 * verification, contact info) while leaving withdrawal rules and
 * type-specific persisted fields to subclasses via the template-method
 * pattern (withdraw / extraFieldsToFileLine / applyPeriodicAdjustment).
 */

#include <string>

class Account {
protected:
    int accountNumber;
    std::string holderName;
    std::string phoneNumber;
    std::string pinHash;
    std::string pinSalt;
    double balance;
    bool active;

public:
    Account(int accountNumber, std::string holderName, std::string phoneNumber,
            std::string pinHash, std::string pinSalt, double initialBalance);

    virtual ~Account() = default;

    // --- Extension points for subclasses ---
    virtual std::string getAccountType() const = 0;
    virtual void withdraw(double amount);                 // base rule: can't exceed balance
    virtual double getWithdrawableLimit() const;           // how much is currently available to withdraw
    virtual std::string extraFieldsToFileLine() const = 0;  // subclass-specific persisted fields
    virtual void applyPeriodicAdjustment() {}               // e.g. interest credit; no-op by default

    // --- Shared behavior ---
    void deposit(double amount);
    bool verifyPin(const std::string& pin) const;
    void changePin(const std::string& newPin);

    // --- Accessors ---
    int getAccountNumber() const { return accountNumber; }
    const std::string& getHolderName() const { return holderName; }
    const std::string& getPhoneNumber() const { return phoneNumber; }
    double getBalance() const { return balance; }
    bool isActive() const { return active; }
    const std::string& getPinHash() const { return pinHash; }
    const std::string& getPinSalt() const { return pinSalt; }

    void setHolderName(const std::string& name) { holderName = name; }
    void setPhoneNumber(const std::string& phone) { phoneNumber = phone; }
    void deactivate() { active = false; }

    // Common part of the persisted line; subclasses append their own fields.
    std::string commonFieldsToFileLine() const;

    void display() const;
};
