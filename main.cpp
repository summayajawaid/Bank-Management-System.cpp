/*
 * main.cpp
 * --------
 * The presentation/UI layer. Talks only to Bank's public API and reacts
 * to the typed exceptions it throws -- it never touches files or account
 * internals directly. Structured as three flows:
 *   1. Open a new account
 *   2. Log into an existing account (PIN-verified session) and transact
 *   3. Admin panel (bank-wide reporting + interest run)
 */

#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

#include "Bank.h"
#include "Exceptions.h"
#include "Utils.h"

namespace {

// Hard-coded demo admin credential. In a real system this would be a
// properly managed account with its own hashed credential store -- this
// exists purely to gate the reporting/admin menu in this demo.
const std::string ADMIN_PASSWORD = "admin123";

void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Thrown when stdin is exhausted (EOF / closed pipe) so callers can unwind
// to a graceful shutdown instead of looping forever trying to re-read.
struct InputClosedException {};

int readInt(const std::string& prompt) {
    int value;
    std::cout << prompt;
    while (!(std::cin >> value)) {
        if (std::cin.eof()) throw InputClosedException{};
        std::cout << "Invalid input. Please enter a whole number: ";
        clearInputBuffer();
    }
    return value;
}

double readDouble(const std::string& prompt) {
    double value;
    std::cout << prompt;
    while (!(std::cin >> value)) {
        if (std::cin.eof()) throw InputClosedException{};
        std::cout << "Invalid input. Please enter a number: ";
        clearInputBuffer();
    }
    return value;
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosedException{};
    return bankutils::trim(line);
}

std::string readPin(const std::string& prompt) {
    while (true) {
        std::string pin = readLine(prompt);
        if (bankutils::isValidPin(pin)) return pin;
        std::cout << "PIN must be 4-6 digits. Please try again.\n";
    }
}

void printMainMenu() {
    std::cout << "\n==============================================\n";
    std::cout << "         BANK MANAGEMENT SYSTEM\n";
    std::cout << "==============================================\n";
    std::cout << "1. Open a New Account\n";
    std::cout << "2. Access My Account (Login)\n";
    std::cout << "3. Admin Panel\n";
    std::cout << "4. Exit\n";
    std::cout << "==============================================\n";
}

void printSessionMenu(int accNo) {
    std::cout << "\n--------- Account #" << accNo << " ---------\n";
    std::cout << "1. View Details\n";
    std::cout << "2. Deposit\n";
    std::cout << "3. Withdraw\n";
    std::cout << "4. Transfer to Another Account\n";
    std::cout << "5. View Transaction History\n";
    std::cout << "6. Update Contact Info\n";
    std::cout << "7. Change PIN\n";
    std::cout << "8. Close Account\n";
    std::cout << "9. Logout\n";
    std::cout << "-----------------------------------\n";
}

void handleOpenAccount(Bank& bank) {
    std::cout << "\nAccount type -- 1) Savings  2) Current: ";
    int type = readInt("");
    clearInputBuffer();

    std::string name = readLine("Full name: ");
    std::string phone = readLine("Phone number: ");
    std::string pin = readPin("Set a 4-6 digit PIN: ");
    double deposit = readDouble("Initial deposit: ");
    clearInputBuffer();

    try {
        if (type == 1) {
            Account& acc = bank.createSavingsAccount(name, phone, pin, deposit);
            std::cout << "\nSavings account created successfully!\n";
            acc.display();
        } else if (type == 2) {
            Account& acc = bank.createCurrentAccount(name, phone, pin, deposit);
            std::cout << "\nCurrent account created successfully!\n";
            acc.display();
        } else {
            std::cout << "Invalid account type selected.\n";
        }
    } catch (const BankException& e) {
        std::cout << "Could not open account: " << e.what() << "\n";
    }
}

void handleSession(Bank& bank, int accNo) {
    bool loggedIn = true;
    while (loggedIn) {
        printSessionMenu(accNo);
        int choice = readInt("Enter your choice: ");
        clearInputBuffer();

        try {
            switch (choice) {
                case 1: {
                    bank.authenticate(accNo, readPin("Confirm your PIN: ")); // re-verify identity, discard ref
                    // authenticate() re-fetches internally; simplest is to just look at all accounts:
                    for (const auto* acc : bank.getAllAccounts()) {
                        if (acc->getAccountNumber() == accNo) { acc->display(); break; }
                    }
                    break;
                }
                case 2: {
                    double amount = readDouble("Amount to deposit: ");
                    bank.deposit(accNo, amount);
                    std::cout << "Deposit successful.\n";
                    break;
                }
                case 3: {
                    double amount = readDouble("Amount to withdraw: ");
                    clearInputBuffer();
                    std::string pin = readPin("Confirm your PIN: ");
                    bank.withdraw(accNo, amount, pin);
                    std::cout << "Withdrawal successful.\n";
                    break;
                }
                case 4: {
                    int toAcc = readInt("Destination account number: ");
                    double amount = readDouble("Amount to transfer: ");
                    clearInputBuffer();
                    std::string pin = readPin("Confirm your PIN: ");
                    bank.transfer(accNo, toAcc, amount, pin);
                    std::cout << "Transfer successful.\n";
                    break;
                }
                case 5: {
                    auto statement = bank.getStatement(accNo);
                    if (statement.empty()) {
                        std::cout << "No transactions yet.\n";
                    } else {
                        std::cout << "\n" << std::left << std::setw(21) << "Timestamp"
                                  << std::setw(14) << "Type" << std::right << std::setw(12) << "Amount"
                                  << std::setw(14) << "Balance" << "   Description\n";
                        std::cout << std::string(85, '-') << "\n";
                        for (const auto& t : statement) t.display();
                    }
                    break;
                }
                case 6: {
                    std::string newName = readLine("New name (blank to keep current): ");
                    std::string newPhone = readLine("New phone (blank to keep current): ");
                    std::string pin = readPin("Confirm your PIN: ");
                    bank.updateContactInfo(accNo, pin, newName, newPhone);
                    std::cout << "Contact info updated.\n";
                    break;
                }
                case 7: {
                    std::string oldPin = readPin("Current PIN: ");
                    std::string newPin = readPin("New PIN: ");
                    bank.changePin(accNo, oldPin, newPin);
                    std::cout << "PIN changed successfully.\n";
                    break;
                }
                case 8: {
                    std::string pin = readPin("Confirm your PIN to close this account: ");
                    bank.closeAccount(accNo, pin);
                    std::cout << "Account closed.\n";
                    loggedIn = false;
                    break;
                }
                case 9:
                    std::cout << "Logging out...\n";
                    loggedIn = false;
                    break;
                default:
                    std::cout << "Invalid choice.\n";
            }
        } catch (const BankException& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
}

void handleLogin(Bank& bank) {
    int accNo = readInt("\nAccount number: ");
    clearInputBuffer();
    std::string pin = readPin("PIN: ");

    try {
        bank.authenticate(accNo, pin);
        std::cout << "Login successful.\n";
        handleSession(bank, accNo);
    } catch (const BankException& e) {
        std::cout << "Login failed: " << e.what() << "\n";
    }
}

void handleAdminPanel(Bank& bank) {
    std::string password = readLine("\nAdmin password: ");
    if (password != ADMIN_PASSWORD) {
        std::cout << "Incorrect admin password.\n";
        return;
    }

    bool inAdmin = true;
    while (inAdmin) {
        std::cout << "\n--------- Admin Panel ---------\n";
        std::cout << "1. View All Accounts\n";
        std::cout << "2. Bank Summary\n";
        std::cout << "3. Apply Monthly Interest (Savings)\n";
        std::cout << "4. Back to Main Menu\n";
        std::cout << "--------------------------------\n";
        int choice = readInt("Enter your choice: ");
        clearInputBuffer();

        switch (choice) {
            case 1: {
                auto all = bank.getAllAccounts();
                if (all.empty()) {
                    std::cout << "No accounts on record.\n";
                    break;
                }
                std::cout << "\n" << std::left << std::setw(10) << "Acc No."
                          << std::setw(20) << "Name" << std::setw(15) << "Phone"
                          << std::setw(10) << "Type" << std::setw(10) << "Status"
                          << std::right << std::setw(12) << "Balance" << "\n";
                std::cout << std::string(77, '-') << "\n";
                for (const auto* acc : all) {
                    std::cout << std::left << std::setw(10) << acc->getAccountNumber()
                              << std::setw(20) << acc->getHolderName()
                              << std::setw(15) << acc->getPhoneNumber()
                              << std::setw(10) << acc->getAccountType()
                              << std::setw(10) << (acc->isActive() ? "Active" : "Closed")
                              << std::right << std::setw(12) << std::fixed << std::setprecision(2)
                              << acc->getBalance() << "\n";
                }
                break;
            }
            case 2: {
                std::cout << "\nActive accounts: " << bank.getActiveAccountCount() << "\n";
                std::cout << std::fixed << std::setprecision(2);
                std::cout << "Total assets held: " << bank.getTotalAssets() << "\n";
                break;
            }
            case 3: {
                int credited = bank.applyMonthlyInterestToAllSavings();
                std::cout << "Interest applied to " << credited << " savings account(s).\n";
                break;
            }
            case 4:
                inAdmin = false;
                break;
            default:
                std::cout << "Invalid choice.\n";
        }
    }
}

} // namespace

int main() {
    Bank bank; // loads existing accounts/transactions from data/ on construction

    std::cout << "Welcome to the Bank Management System.\n";

    try {
        bool running = true;
        while (running) {
            printMainMenu();
            int choice = readInt("Enter your choice: ");
            clearInputBuffer();

            switch (choice) {
                case 1: handleOpenAccount(bank); break;
                case 2: handleLogin(bank); break;
                case 3: handleAdminPanel(bank); break;
                case 4:
                    std::cout << "\nThank you for banking with us. Goodbye!\n";
                    running = false;
                    break;
                default:
                    std::cout << "Invalid choice. Please try again.\n";
            }
        }
    } catch (const InputClosedException&) {
        // Input stream closed (e.g. piped input ran out, or Ctrl+D at a prompt).
        // Exit gracefully rather than crashing or looping forever.
        std::cout << "\nInput closed. Goodbye!\n";
    }

    return 0;
}
