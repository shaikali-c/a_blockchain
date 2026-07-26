#pragma once

// Precompiled header for stable, frequently used, and relatively expensive headers.
// Keep project headers out of this file so normal code edits do not invalidate the PCH.

// Stable third-party dependencies used by core translation units
#include <asio.hpp>
#include <leveldb/db.h>
#include <sodium.h>

// Standard library
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <expected>
#include <filesystem>
#include <format>
#include <functional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <ranges>
#include <set>
#include <shared_mutex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
