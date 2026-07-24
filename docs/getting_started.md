# Getting started

This guide walks you through building, running, and interacting with an
Axis node.

## Prerequisites

### Required dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| C++ compiler | C++23 (GCC 14+, Clang 18+) | Compiling the codebase |
| CMake | 3.22+ | Build system |
| libsodium | 1.0.18+ | Cryptographic operations |
| LevelDB | 1.23+ | Persistent storage |
| Asio | 1.28+ (header-only) | Networking + coroutines |

### Installing on Debian/Ubuntu

```bash
sudo apt update
sudo apt install build-essential cmake libsodium-dev libleveldb-dev
```

Asio is header-only. If your distribution doesn't package it:

```bash
# Download to /usr/local/include
sudo wget -O /usr/local/include/asio.hpp \
  https://raw.githubusercontent.com/chriskohlhoff/asio/master/asio/include/asio.hpp
```

Or install via apt if available:
```bash
sudo apt install libasio-dev  # may be older version
```

### Installing on Arch Linux

```bash
sudo pacman -S base-devel cmake libsodium leveldb asio
```

### Checking your compiler

```bash
g++ --version   # needs 14+
# or
clang++ --version  # needs 18+
```

## Building

### Quick build

```bash
cd axis
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces:
- `build/axisd` — the node executable
- `build/libaxis_core.a` — the core library (for linking tests)
- `build/axis_core_tests` — the test suite

### Debug build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Debug builds include assertions and no optimizations, making them easier to
step through with a debugger.

### Build configuration

`CMakeLists.txt` auto-detects dependencies via PkgConfig. Key settings:

```cmake
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Uses -Wall -Wextra -Wpedantic -Werror
```

## Running

### Start the node

```bash
./build/axisd
```

This will:
1. Create `./axis_data/blocks/` and `./axis_data/pool/` directories
2. If no blocks exist, create the genesis block (height 0)
3. Rebuild the UTXO set from stored blocks
4. Reload the mempool from the pool database
5. Start the TCP server on port 8080

### Output

```
Axis node running on port 8080
```

The node runs until you press Ctrl+C. It handles:
- UTXO queries (GetUTXOs)
- Transaction submissions (CreateTransaction)
- Transaction lookups (GetTx, GetMempoolTx)
- Block range queries (GetBlockRange)

### Command-line options

```
--help     Print usage message and exit
```

Currently there is no `--port` or `--data-dir` flag. These are hardcoded.

## Interacting with the node

### Using netcat / ncat

```bash
# Get UTXOs for the genesis address
python3 -c "
import socket, struct
s = socket.socket()
s.connect(('127.0.0.1', 8080))
# Header: magic(4) + type(1) + payload_len(4 BE)
# GetUTXOs payload: address(20)
addr = bytes.fromhex('f45a20e043b01f65638a46831ce79b8fec3f6737')
payload = addr
s.sendall(struct.pack('<IB', 0xDEADBEEF, 3))
s.sendall(struct.pack('>I', len(payload)))
s.sendall(payload)
# Read response: header + payload
resp = s.recv(4096)
print('Response hex:', resp.hex())
s.close()
"
```

Expected response (hex):
```
deadbeef 04 000000XX
[varint count] [OutPoint...] [TxOutput...]
```

### Using a Python client

```python
#!/usr/bin/env python3
"""Minimal Axis client."""
import socket, struct

class AxisClient:
    def __init__(self, host='127.0.0.1', port=8080):
        self.sock = socket.socket()
        self.sock.connect((host, port))

    def _send(self, msg_type, payload):
        self.sock.sendall(struct.pack('<IB', 0xDEADBEEF, msg_type))
        self.sock.sendall(struct.pack('>I', len(payload)))
        self.sock.sendall(payload)

    def _recv(self):
        header = self.sock.recv(9)
        if len(header) < 9: return None
        magic, type_, plen = struct.unpack('<IB', header[:5]) + (
            struct.unpack('>I', header[5:9])[0],)
        payload = self.sock.recv(plen) if plen else b''
        return type_, payload

    def get_utxos(self, addr_hex):
        addr = bytes.fromhex(addr_hex)
        self._send(3, addr)  # GetUTXOs
        return self._recv()

    def close(self):
        self.sock.close()

client = AxisClient()
type_, payload = client.get_utxos(
    'f45a20e043b01f65638a46831ce79b8fec3f6737')
print(f'Response type: {type_}, payload: {payload.hex()}')
client.close()
```

## Running tests

```bash
cd axis
cmake -B build
cmake --build build
ctest --test-dir build -V
```

Or run the test binary directly:

```bash
./build/axis_core_tests
```

Expected output:

```
[==========] Running 5 tests
[ RUN      ] TransactionSerializationRoundtrip
[       OK ] TransactionSerializationRoundtrip
[ RUN      ] BlockSerializationRoundtrip
[       OK ] BlockSerializationRoundtrip
[ RUN      ] ChainUTXOQuery
[       OK ] ChainUTXOQuery
[ RUN      ] TransactionSignatureValidation
[       OK ] TransactionSignatureValidation
[ RUN      ] BadSignatureRejected
[       OK ] BadSignatureRejected
[==========] 5 tests passed (0 ms)
```

## Cleaning up

```bash
# Remove build artifacts
rm -rf build/

# Remove blockchain data
rm -rf axis_data/
```

## Next steps

- Read `docs/architecture.md` for the high-level design
- Read `docs/transaction_lifecycle.md` to understand how transactions flow
- Read `docs/developer_guide.md` for contribution guidelines
- Read `docs/serialization.md` if you need to add a new message type
