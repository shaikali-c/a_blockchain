# Contributing

## Before Changing Code

1. Read the relevant documentation in `docs/`.
2. Identify the owning module for your change.
3. Check whether behavior is protocol-visible or persistence-visible.
4. Add or update tests when behavior changes.
5. Update documentation in the same change.

## Development Workflow

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Documentation Requirements

Update docs when changing:

| Change | Docs to update |
| --- | --- |
| TCP packet layout | `Protocol.md`, `api/ProtocolPackets.md`, `Serialization.md` if object layout changes |
| HTTP route | `Networking.md`, `api/PublicAPI.md` |
| Chain validation | `Validation.md`, relevant component docs |
| Database key/value layout | `Storage.md`, `Database.md` |
| Threading/lifetime | `Threading.md`, `Architecture.md` |
| New class/module | `ProjectStructure.md`, `classes/`, architecture diagrams |

## Testing Expectations

At minimum, run `axis_core_tests` through CTest. For networking changes, add or manually run integration checks for:

- valid request/response,
- malformed payload,
- too-short payload,
- unknown route/message where relevant,
- persistence across restart when state is stored.

## Review Checklist

- Does the change preserve transaction/block hash invariants?
- Are all state mutations protected by the right lock?
- Are network payload sizes bounded before allocation?
- Does persistence remain recoverable after restart?
- Are errors surfaced consistently to TCP/HTTP clients?
- Are docs updated to match implementation exactly?

## Non-Goals for Small Changes

Avoid mixing unrelated improvements into focused fixes. For example, do not refactor all JSON serialization while adding one endpoint unless the endpoint requires it.
