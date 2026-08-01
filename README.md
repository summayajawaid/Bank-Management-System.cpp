# Bank Management System (C++)

A layered, multi-file bank management system demonstrating professional
C++ project structure: OOP with inheritance, custom exceptions, PIN
authentication, transaction logging, and flat-file persistence.

## Architecture

```
include/            Public headers (one class per file)
  Exceptions.h       Custom exception hierarchy (BankException and subtypes)
  Utils.h            Validation, timestamps, salted PIN hashing
  Transaction.h       Immutable transaction record + (de)serialization
  Account.h          Abstract base class (template-method pattern)
  SavingsAccount.h   Minimum balance + monthly interest
  CurrentAccount.h   Overdraft-enabled checking account
  FileManager.h      All raw file I/O isolated behind one class
  Logger.h           Append-only audit log, separate from the ledger
  Bank.h             Service layer: owns accounts, orchestrates operations

src/                 Implementations + main.cpp (CLI / presentation layer)
data/                Created at runtime: accounts.dat, transactions.dat, audit.log
```

**Design choices:**
- `Account` is abstract; `SavingsAccount` and `CurrentAccount` override
  withdrawal rules (`withdraw`, `getWithdrawableLimit`) and their persisted
  fields (`extraFieldsToFileLine`) via the template-method pattern.
- Accounts are owned by `std::unique_ptr` inside `Bank`, so there is no
  manual memory management and no leaks.
- All errors are typed exceptions (`InsufficientFundsException`,
  `AuthenticationException`, etc.) instead of error codes or magic return
  values, so the UI layer can catch `BankException` and print a precise
  message.
- Transfers withdraw first, then deposit, and roll back the debit if the
  credit fails, keeping the two legs consistent even on error.
- PINs are never stored in plaintext -- they're salted and hashed
  (`Utils::hashPin`). This is a **demonstration** hash, not a real
  cryptographic KDF; see the note in `Utils.h`.
- `main.cpp` never touches files or account internals directly -- only
  `Bank`'s public API, keeping the UI layer thin and swappable.

## Build

With CMake:
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
./bank
```

Or directly with g++:
```bash
g++ -std=c++17 -O2 -Iinclude src/*.cpp -o bank
./bank
```

## Usage

- **Open a New Account** -- choose Savings or Current, set a 4-6 digit PIN.
- **Access My Account** -- log in with account number + PIN to deposit,
  withdraw, transfer, view statement, update contact info, change PIN, or
  close the account.
- **Admin Panel** -- password `admin123` (demo only). View all accounts,
  bank-wide totals, and run the monthly interest job for savings accounts.

Data persists in `data/` between runs.

## Account rules

| Type    | Withdrawal rule                          | Interest |
|---------|-------------------------------------------|----------|
| Savings | Cannot go below minimum balance (default $500) | 4%/yr, credited monthly via Admin Panel |
| Current | Can overdraw up to overdraft limit (default $10,000) | None |

## Notes on scope

This is a learning/demo project, not production banking software:
- The PIN hash and hard-coded admin password are illustrative, not secure.
- Storage is flat pipe-delimited files, not a transactional database, so
  concurrent access from multiple processes isn't safe.
