cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(axis)

find_package(Threads REQUIRED)

find_package(leveldb CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Crow CONFIG REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium>=1.0.18)
find_package(nlohmann_json REQUIRED)
find_package(asio CONFIG QUIET)
if(NOT asio_FOUND)
    pkg_check_modules(ASIO REQUIRED asio)
endif()

add_library(axis_core
    "src/chain.cpp"
    "src/block.cpp"
    "src/tx.cpp"
    "src/crypto.cpp"
    "src/net.cpp"
    "src/web.cpp"
)

target_include_directories(axis_core
    PUBLIC include
    PRIVATE ${LIBSODIUM_INCLUDE_DIRS}
    PRIVATE ${ASIO_INCLUDE_DIRS}
)
target_link_libraries(axis_core
    PRIVATE
    leveldb::leveldb
    nlohmann_json::nlohmann_json
    ${LIBSODIUM_LIBRARIES}
    Crow::Crow
)

target_precompile_headers(axis_core
    PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/axis/pch.hpp>"
)

add_executable(axisd "src/main.cpp")
target_link_libraries(axisd PRIVATE axis_core)
target_precompile_headers(axisd
    PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/axis/pch.hpp>"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(axis_core_tests "tests/core_serialization_tests.cpp")
    target_link_libraries(axis_core_tests PRIVATE axis_core)
    target_precompile_headers(axis_core_tests
        PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/axis/pch.hpp>"
    )
    add_test(NAME axis_core_tests COMMAND axis_core_tests)
endif()
