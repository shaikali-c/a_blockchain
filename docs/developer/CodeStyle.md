# Code Style

Axis is written in modern C++23 with a small, direct style. The code favors readability and explicit data structures over abstraction.

## General Style

- Use standard C++23.
- Keep project headers under `include/axis/`.
- Keep implementation files under `src/` with one implementation file per main header.
- Prefer simple value types (`std::array`, `std::vector`, `std::unordered_map`) for blockchain data.
- Prefer clear validation steps over compact clever expressions.
- Keep networking code in `net.cpp`/`web.cpp` and blockchain state changes in `chain.cpp`.

## Current Naming Conventions

| Kind | Style | Examples |
| --- | --- | --- |
| Classes/structs | PascalCase | `Chain`, `Transaction`, `BlockHeader`, `Writer` |
| Functions/methods | snake_case | `add_tx`, `get_utxos`, `compute_merkle_root` |
| Private data members | trailing underscore | `blocks_`, `pool_db_`, `cached_hash_` |
| Constants | mixed current style | `UNITS`, `MINER_REWARD`, `kMaxRawTxHexChars` |
| Enum values | PascalCase | `TxError::BadSignature`, `MsgType::CreateBlock` |

## Header Policy

- Public declarations live in `include/axis/*.h`.
- `types.h` and `util.h` are header-only.
- `pch.hpp` should contain stable third-party and standard headers only. Do not add project headers to the PCH unless you intentionally accept broader rebuilds.

## Error Handling Style

- Use enum return values for expected blockchain validation failures.
- Use exceptions for unrecoverable storage/parser infrastructure failures.
- Convert exceptions to protocol errors at network/API boundaries.
- Keep error strings stable when clients may depend on them.

## Serialization Style

- Use `Writer`/`Reader` for existing binary layouts.
- Always document new fields in `docs/Serialization.md` and `docs/api/ProtocolPackets.md`.
- Prefer explicit length prefixes for variable-size fields.
- If adding public protocol fields, consider explicit endian encoding rather than expanding native-endian assumptions.

## Concurrency Style

- Access `Chain` state through public methods that lock internally.
- Do not mutate `Chain::pool_` directly even though it is currently public.
- Do not expose mutable references to `blocks_`, `utxo_`, or mempool maps.
- Avoid holding locks while calling external callbacks or performing slow network I/O.

## Comments

Add comments for non-obvious invariants, protocol compatibility constraints, or consensus rules. Avoid comments that simply restate the next line of code.
