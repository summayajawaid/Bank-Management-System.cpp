cmake_minimum_required(VERSION 3.10)
project(BankManagementSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(bank
    src/main.cpp
    src/Bank.cpp
    src/Account.cpp
    src/SavingsAccount.cpp
    src/CurrentAccount.cpp
    src/Transaction.cpp
    src/FileManager.cpp
    src/Logger.cpp
    src/Utils.cpp
)

target_include_directories(bank PRIVATE include)
