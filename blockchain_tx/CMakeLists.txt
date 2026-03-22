cmake_minimum_required(VERSION 3.16)

project(blockchain_tx)

find_package(leveldb CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

add_executable(blockchain_tx
    
    "src/databaseManager.cpp"
    "src/keys.cpp"
    "src/common.cpp"
    "src/transaction.cpp"
    "src/blockchain.cpp"
    "src/pch.cpp" "src/main.cpp" "include/logger.h" "src/logger.cpp")

target_compile_features(blockchain_tx PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)
target_link_libraries(blockchain_tx PRIVATE leveldb::leveldb unofficial-sodium::sodium)