# Configuration

Axis currently has minimal runtime configuration. Most values are hardcoded in source.

## Hardcoded Runtime Values

| Setting | Current value | Location |
| --- | --- | --- |
| TCP port | `8889` | `src/main.cpp`, `Server server{chain, 8889, ...}` |
| HTTP/WebSocket port | `8080` | `src/main.cpp`, `WebServer web{chain, 8080}` |
| Blocks DB path | `blocks` | `Chain::Chain()` |
| Pool DB path | `pool` | `Chain::Chain()` |
| Difficulty | `3` leading zero bytes | `Chain::difficulty_` default member initializer |
| Unit scale | `1,000,000` base units per AXIS | `Chain::UNITS`, `format_amount()` default |
| Miner reward constant | `3 * UNITS` | `Chain::MINER_REWARD`, currently not enforced in block submission |
| Genesis reward | `15 * UNITS` | `Chain::create_genesis()` |
| Genesis recipient | `f45a20e043b01f65638a46831ce79b8fec3f6737` | `GENESIS_ADDR` in `src/chain.cpp` |
| Genesis timestamp | `1781545365` | `Chain::create_genesis()` |
| Genesis nonce | `31496` | `Chain::create_genesis()` |
| HTTP max rawTx hex | `128 * 1024` chars | `kMaxRawTxHexChars` in `src/web.cpp` |
| `/api/blocks` default count | `10` | `kDefaultBlockCount` in `src/web.cpp` |
| `/api/blocks` max count | `100` | `kMaxBlockCount` in `src/web.cpp` |

## Build Configuration

| Setting | Source |
| --- | --- |
| C++ standard | C++23 in `CMakeLists.txt` |
| Tests enabled | `BUILD_TESTING` via CTest |
| Precompiled headers | `include/axis/pch.hpp` |

## Environment Variables

No environment variables are read by the current code.

## Command-Line Arguments

`main()` accepts no arguments.

## Configuration Gaps

Future production-quality configuration would likely include:

- ports and bind addresses,
- database paths,
- genesis/network identifier,
- difficulty/consensus parameters,
- logging level,
- CORS origins,
- TLS/reverse-proxy settings,
- max payload sizes/counts,
- mempool size/eviction policy.
