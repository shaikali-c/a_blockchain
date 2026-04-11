cmake_minimum_required(VERSION 3.16)

project(blockchain_tx)

add_definitions(-DWIN32_LEAN_AND_MEAN -D_WINSOCKAPI_)
find_package(leveldb CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(Drogon CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
target_link_libraries(blockchain_tx PRIVATE leveldb::leveldb unofficial-sodium::sodium Drogon::Drogon)
