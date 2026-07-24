# Developer guide

## Code conventions

### Style

Axis follows a consistent C++ style:

| Convention | Rule |
|------------|------|
| Naming (types) | PascalCase: `Transaction`, `OutPoint`, `TxError` |
| Naming (functions/methods) | snake_case: `derive_address`, `add_tx`, `get_utxos` |
| Naming (variables) | snake_case: `tip_hash`, `pool_spent_` |
| Naming (private members) | trailing underscore: `inputs_`, `header_` |
| Naming (constants) | UPPER_SNAKE: `GENESIS_ADDR`, `UNITS` |
| Indentation | 4 spaces (no tabs) |
| Braces | Same line: `if (...) {`, `} else {` |
| Line length | 80 character soft limit |
| Includes | Sort: standard → library → project |
| Comments | None in headers; minimal in .cpp |

### Header order

```cpp
// Standard library first
#include <array>
#include <cstdint>
#include <vector>

// Third-party library headers
#include <leveldb/db.h>

// Project headers (from include/)
#include <axis/chain.h>
#include <axis/types.h>
```

### Namespace

There is no namespace. The project uses the `Axis` prefix convention
for classes (`AxisTransaction`, etc.) — actually, it doesn't use any
prefix or namespace. All types are in the global namespace.

This is intentional: the project is small enough that name collisions
are unlikely. Adding a namespace would add indentation and qualification
without benefit.

If you add new types, keep them in the global namespace. If the project
grows significantly, consider adding an `axis::` namespace.

## Adding a new message type

1. Add the new value to `MsgType` in `include/axis/net.h`
2. Add a handler method to `Server` in `include/axis/net.h`
3. Implement the handler in `src/net.cpp`
4. Add dispatch logic in `Server::session`

```cpp
// In session() switch:
case MsgType::NewMessage:
    co_await on_new_message(sock, payload);
    break;
```

## Adding a new field to Transaction

1. Add the field to `Transaction` in `include/axis/tx.h`
2. Update `Transaction::serialize()` — include the new field
3. Update `Transaction::compute_hash()` — include the new field in txid
4. Update the span constructor to deserialize the new field
5. Update `SignedTransaction::serialize()` — include the new field
6. Update `SignedTransaction` span constructor
7. Update tests in `tests/core_serialization_tests.cpp`

**Important:** Adding a field to the txid computation changes the txid of
every past transaction. This is a hard fork. In production, you'd use a
new version field.

## Adding a new field to Block header

1. Add the field to `Block::Header` in `include/axis/block.h`
2. Update `Block::hash()` — include the new field in the header hash
3. Update `Block::serialize()` — include the new field
4. Update the span constructor

**Important:** Changing the block header hash computation changes the
entire chain's hash values. This is a hard fork.

## Writing tests

Tests use a simple framework in `tests/core_serialization_tests.cpp`. The
framework provides:

- `TEST(name)` — define a test function
- `CHECK(expr)` — assert a condition
- `CHECK_EQ(a, b)` — assert equality
- `RUN_TEST(name)` — run a test and report

### Example

```cpp
TEST(MyNewTest) {
    // Arrange
    auto pk = PublicKey{};
    auto sk = PrivateKey{};
    generate_keypair(pk, sk);

    // Act
    Hash h = blake2b({pk.data(), pk.size()});

    // Assert
    CHECK(h != Hash{});
}
```

To add it:

```cpp
int main() {
    RUN_TEST(MyNewTest);
    // ... existing tests ...
}
```

### Test patterns

- **Serialization roundtrip:** Create object → serialize → deserialize →
  verify equal
- **Validation tests:** Create valid/invalid inputs and verify the
  expected error code
- **Edge cases:** Empty transactions, maximum values, zero values

## Debugging

### Build for debug

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Debug builds:
- Include all source-level debug information (`-g`)
- Disable optimizations (`-O0`)
- Enable assertions

### Common issues

**"Unable to open database"**: Delete `axis_data/` and restart.

**"Undefined reference to `blake2b`"**: libsodium not linked. Check
`pkg-config --libs libsodium`.

**"Coroutine support not found"**: Compiler too old or missing `-fcoroutines`.
GCC 14+ required.

### GDB

```bash
gdb --args ./build/axisd
(gdb) break Chain::add_tx
(gdb) run
# Trigger a CreateTransaction from another terminal
(gdb) bt
(gdb) print stx.tx.txid()
```

## Performance considerations

### Current bottlenecks

1. **UTXO set iteration** (`get_utxos`): Scans the entire UTXO set. With
   millions of UTXOs, this would be slow. A secondary index (address →
   list of OutPoints) would help.

2. **Startup replay**: All blocks are replayed on startup. For long chains,
   this takes linear time.

3. **Single-threaded**: All processing happens on one I/O thread.
   Validation and serialization are CPU-bound and block other connections.

### Future optimizations

- **UTXO cache/index**: Maintain a map from address to list of OutPoints for
  O(1) per-address queries.
- **Checkpoint-based startup**: Periodically snapshot the UTXO set and
  replay only blocks after the checkpoint.
- **Multiple threads**: Use `asio::io_context` with multiple threads, or
  offload validation to a thread pool.

## Adding dependencies

### If you need a new library

1. Add the `pkg_check_modules` call in `CMakeLists.txt`
2. Add the target link libraries
3. Add include directories
4. Document the dependency in `docs/getting_started.md`

### Keep dependencies minimal

Axis has three external dependencies: libsodium (crypto), LevelDB
(storage), Asio (networking). Each is essential and adds no redundancy.

Before adding a new dependency, ask:
- Can I implement this with existing dependencies?
- Is the dependency widely available on all target platforms?
- Is the dependency's API stable and well-documented?
- Does the dependency introduce licensing concerns?
