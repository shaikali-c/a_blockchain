cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

project(blockchain_tx)

find_package(Crow CONFIG REQUIRED)
find_package(leveldb CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBSODIUM REQUIRED libsodium>=1.0.18)

add_library(axis_core
    "src/storage/database_manager.cpp"
    "src/core/common.cpp"
    "src/blockchain/transaction.cpp"
    "src/blockchain/blockchain.cpp"
    "src/pch.cpp"
    "src/core/logger.cpp"
    "src/core/pretty_print.cpp"
    "src/crypto/cryptography.cpp"
    "src/blockchain/block.cpp"
    )

target_include_directories(axis_core PUBLIC include PRIVATE ${LIBSODIUM_INCLUDE_DIRS})
target_precompile_headers(axis_core PRIVATE include/pch.h)
target_link_libraries(axis_core
    PRIVATE
    leveldb::leveldb
    nlohmann_json::nlohmann_json
    ${LIBSODIUM_LIBRARIES}
)

add_executable(blockchain_tx "src/main.cpp")
target_link_libraries(blockchain_tx PRIVATE axis_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(axis_core_tests "tests/core_serialization_tests.cpp")
    target_link_libraries(axis_core_tests PRIVATE axis_core)
    add_test(NAME axis_core_tests COMMAND axis_core_tests)
endif()
