// pch.h
#pragma once


// 🔥 CRITICAL FIX (must be FIRST)
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_   // 🚫 block winsock.h

// ------------------------------
// C++ Standard Library
// ------------------------------
#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ------------------------------
// Third-Party Libraries
// ------------------------------
#include <leveldb/db.h>
#include <tabulate.hpp>
#include <drogon/drogon.h>