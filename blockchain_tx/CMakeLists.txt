cmake_minimum_required(VERSION 3.16)

project(blockchain_tx)

find_package(leveldb CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

add_executable(blockchain_tx
    "src/main.cpp"
    "include/blockchain_tx.h"
 "include/databaseManager.h" "src/databaseManager.cpp" "include/cryptography.h" "include/keys.h" "include/pch.h" "src/keys.cpp" "include/common.h" "src/common.cpp" "include/transaction.h" "src/transaction.cpp" "include/blockchain.h" "src/blockchain.cpp")

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)
target_link_libraries(blockchain_tx PRIVATE leveldb::leveldb unofficial-sodium::sodium)