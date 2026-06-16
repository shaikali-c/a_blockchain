cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(blockchain_tx)

find_package(Crow CONFIG REQUIRED)
find_package(leveldb CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(blockchain_tx
    "src/databaseManager.cpp"
    "src/keys.cpp"
    "src/common.cpp"
    "src/transaction.cpp"
    "src/blockchain.cpp"
    "src/pch.cpp"
    "src/main.cpp"
    "src/logger.cpp"
    "src/cryptography.cpp"
    "src/block.cpp"
    )

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    leveldb::leveldb
    unofficial-sodium::sodium
    nlohmann_json::nlohmann_json
)
