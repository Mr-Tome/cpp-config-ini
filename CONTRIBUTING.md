# Contributing to cpp-config-ini

## Building

```bash
./configure        # first-time setup only
./make             # builds project, examples, and tests
```

The build system is CMake 3.20+ with Ninja. All targets require C++20.

## Running the Tests

```bash
cd build && ctest
```

All 10 suites must pass. To run a single suite directly:

```bash
./build/tests/test_config_item
```

## Adding a Test

1. Add your test cases to the relevant file in `tests/` using the existing macros:

   ```cpp
   REQUIRE(expr)
   REQUIRE_EQ(a, b)
   REQUIRE_THROWS(expr)
   REQUIRE_NO_THROW(expr)
   ```

2. Register each test with `runTests()` in that file's `main()`.

3. If you need a new test file, add it to `tests/CMakeLists.txt`:

   ```cmake
   add_executable(test_my_feature test_my_feature.cpp)
   target_link_libraries(test_my_feature PRIVATE source_directory_lib)
   target_include_directories(test_my_feature PRIVATE ${CMAKE_SOURCE_DIR}/source)
   add_test(NAME test_my_feature COMMAND test_my_feature)
   ```

Use `TempFile` from `test_framework.hpp` for any test that writes to disk — it cleans up automatically.

## Adding an Example

Drop a `.cpp` file anywhere under `examples/`. CMake picks it up automatically at build time — no `CMakeLists.txt` changes needed. Each example must have its own `main()`.

## Code Conventions

**Language**: C++20. Use concepts, `std::string_view`, and ranges where they clarify intent.

**Namespaces**:
- `ConfigLib` — public API
- `ConfigLib::Internal` — implementation details; do not use from user code
- `ValidationRules` — standalone validation helpers

**CRTP pattern**: User-facing classes (`ConfigReader`, `ConfigType`) use CRTP. When adding a new base template, follow the existing cast pattern: `static_cast<Derived&>(*this)`.

**Compiler warnings are errors.** The full warning set includes `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror` and several others. Fix warnings rather than suppressing them.

**Error handling**: Throw `std::runtime_error` with a descriptive message for user-facing errors. Do not use `std::cerr` inside library code; use the logger callback (`ConfigLib::setLogger()`) instead.

**Comments**: Only comment the *why*, not the *what*. Skip comments when the code is self-explanatory.

**`[[nodiscard]]`**: Apply to any function whose return value the caller must not ignore.

## Validation Rules

To add a new built-in rule, implement the `Rule` interface in `common/validation_rules.hpp` and expose it as a `const` singleton (stateless) or factory function (parameterized), following the pattern of `greaterThanZero` and `BetweenValues`.

## Custom Type Registration

Custom types inherit `ConfigLib::ConfigType<T>` and implement:

| Method | Signature | Notes |
|---|---|---|
| `typeName` | `static const char* typeName()` | unique string identifier |
| `toString` | `std::string toString() const` | serialized form stored in INI |
| `fromString` | `static T fromString(const std::string&)` | must round-trip with `toString` |

Registration is automatic via the CRTP base.

## Schema Migration

When changing field names or value formats across versions:

1. Increment `getSchemaVersion()` in your `ConfigReader` subclass.
2. Add a `SchemaMigration` entry in `getMigrations()` that covers each version step.
3. Add a test in `tests/test_schema_migration.cpp` covering the old → new transition.

See `examples/cli_and_ini/color_migration.cpp` for a complete worked example.

## Pull Requests

- Run `ctest` and confirm all tests pass before opening a PR.
- Keep commits focused; one logical change per commit.
- Write a commit message that explains *why* the change is made, not just what files changed.
