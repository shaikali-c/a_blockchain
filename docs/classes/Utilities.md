# Utility Functions

Source: `include/axis/util.h`

## Logging

| Function | Output |
| --- | --- |
| `logging::info(msg)` | `[INFO] msg` to `std::cout`. |
| `logging::err(msg)` | `[ERR] msg` to `std::cout`. |
| `logging::reject(msg)` | `[REJ] msg` to `std::cout`. |

## Formatting and Conversion

| Function | Purpose |
| --- | --- |
| `to_hex(array)` | Convert fixed byte array to lowercase hex using libsodium. |
| `from_hex<N>(hex)` | Convert exact-length hex to `std::array<uint8_t,N>`, throwing on invalid input. |
| `short_hex(hash)` | Currently returns the full hash hex; shortening is commented out. |
| `short_addr(address)` | Returns first 8 and last 8 hex chars separated by `..`. |
| `format_amount(amount, units)` | Formats base units with six fractional digits by default. |
| `format_timestamp(ts)` | Formats UTC timestamp with `std::gmtime` and `strftime`. |

## Notes

`format_timestamp()` uses `std::gmtime`, which returns a pointer to static storage and is not generally thread-safe. It is used for logging/pretty output, not consensus.
