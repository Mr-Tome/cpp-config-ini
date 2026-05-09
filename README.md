# cpp-config-ini

A C++20 library for type-safe, schema-driven INI configuration files with CLI override support, validation rules, and schema migration.

## Features

- **Type-safe access** — compile-time enforcement via C++20 concepts; `getValue<T>()` returns the exact type you declared
- **INI persistence** — reads and writes standard INI files automatically
- **CLI overrides** — command-line arguments override any config value at runtime
- **Custom types** — register your own types by implementing `fromString()` / `toString()` / `typeName()`
- **Validation rules** — attach constraints (range checks, allowed lists, custom predicates) to any field
- **Schema migration** — version your schema and define field renames or value transformations between versions
- **Volatile fields** — mark fields as CLI-only so they are never persisted to disk

## Requirements

- GCC with C++20 support
- CMake 3.20+
- Ninja build system

The `./configure` script will download and set up CMake and GCC if they are not already present.

## Getting Started

```mermaid
flowchart LR
    A["git clone"] --> B["./configure\ninstall CMake · GCC"]
    B --> C["./make\nbuild project + examples"]
    C --> D["./run\nmain demo"]
    C --> E["cd build && ctest\nall test suites"]
```

```bash
git clone https://github.com/Mr-Tome/cpp-config-ini.git
cd cpp-config-ini
./configure        # install build dependencies (~/.configuration-dependencies/)
./make             # compile the project and all examples
./run              # run the main demo
```

### Running Tests

```bash
cd build && ctest
```

All 10 test suites must pass before submitting changes.

### Cleaning Up

```bash
./configure clean  # remove downloaded dependencies
./make clean       # remove build artifacts
```

## Architecture

```mermaid
flowchart TB
    subgraph User["Your Config Class"]
        UC["struct MyConfig : ConfigReader&lt;MyConfig, INI, CLI&gt;"]
    end

    subgraph Schema["Schema  —  getConfigSections()"]
        S["ConfigSection"] --> I["ConfigItem::make&lt;T&gt;()"]
        I --> V["Validation Rule"]
        I --> CT["Type\n(int · double · string · vector · custom)"]
    end

    subgraph Layers["Persistence Layers"]
        INI["INI Layer"] <-->|"read / write"| File[(".ini file")]
        CLI["CLI Layer"] -->|"parse"| Argv(["argv[]"])
    end

    API["getValue&lt;T&gt;()  ·  setValue&lt;T&gt;()  ·  saveConfig()"]

    UC --> Schema
    UC --> Layers
    Schema --> API
    Layers --> API
```

## Implementing a Config Class

Inherit from `ConfigLib::ConfigReader<YourClass, Modes...>` and implement the methods below. The table shows everything you can define at a glance:

| Method | Required? | Purpose |
|---|---|---|
| `getConfigFilePath()` | **Yes** (INI mode) | Path to the `.ini` file to read/write |
| `getConfigSections()` | **Yes** | Declares your schema — sections, keys, types, and defaults |
| `getSchemaVersion()` | No | Enables schema versioning; return the current version number |
| `getMigrations()` | No | Defines renames and value transforms when upgrading from an older file |
| `getOrphanedConfigItemPolicy()` | No | What to do with keys in the file that are no longer in the schema |
| `flattenCLIArgs()` | No | Allow `--key=value` without section prefix (default: `true`) |

---

### Required

Pick a persistence mode — `INI`, `CLI`, or both — as template arguments.

**INI only** (file persistence, no command-line overrides):

```cpp
#include "config_library/config_reader/config_reader.hpp"

struct MyConfig : public ConfigLib::ConfigReader<MyConfig, ConfigLib::INI>
{
    std::string getConfigFilePath() const { return "my_app.ini"; }

    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {
            { "Server", {
                ConfigLib::ConfigItem::make<std::string>("host", std::string("localhost"), "Server hostname", nullptr),
                ConfigLib::ConfigItem::make<int>("port", 8080, "Server port", nullptr)
            }}
        };
    }
};
```

**INI + CLI** (file persistence with command-line overrides — forward `argc`/`argv` in the constructor):

```cpp
struct MyConfig : public ConfigLib::ConfigReader<MyConfig, ConfigLib::INI, ConfigLib::CLI>
{
    MyConfig(int argc, char* argv[])
        : ConfigLib::ConfigReader<MyConfig, ConfigLib::INI, ConfigLib::CLI>(argc, argv) {}

    std::string getConfigFilePath() const { return "my_app.ini"; }

    std::vector<ConfigLib::ConfigSection> getConfigSections() const { /* same as above */ }
};
```

On first run, `my_app.ini` is created with your defaults. On subsequent runs it is loaded from disk. CLI arguments override any value at runtime: `./my_app --Server.port=9090`

**Reading and writing values at runtime:**

```cpp
MyConfig config(argc, argv);

std::string host = config.getValue<std::string>("Server", "host");
int port         = config.getValue<int>("Server", "port");

config.setValue<int>("Server", "port", 9090);
config.saveConfig();
```

---

### Optional

#### Schema versioning and migration

When you rename a field or change a value format, increment `getSchemaVersion()` and describe the upgrade in `getMigrations()`. The library applies the steps automatically when it loads an older file.

```cpp
uint32_t getSchemaVersion() const { return 2; }

std::vector<ConfigLib::SchemaMigration> getMigrations() const
{
    return {
        ConfigLib::Migration::rename(1, 2, "Server", "hostname", "Server", "host")
    };
}
```

See `examples/cli_and_ini/color_migration.cpp` for a full walkthrough including value transforms.

#### Orphaned item policy

Controls what happens to keys in the INI file that no longer exist in the schema:

```cpp
ConfigLib::OrphanedConfigItemPolicy getOrphanedConfigItemPolicy() const
{
    return ConfigLib::OrphanedConfigItemPolicy::CommentOut; // default
    // Other options: Remove | RuntimeError
}
```

#### Flat CLI args

When `true` (the default), a CLI argument can omit the section prefix if the key is unambiguous across all sections — `--port=9090` resolves to `Server.port` automatically.

```cpp
bool flattenCLIArgs() const { return false; } // require --Server.port=9090
```

#### Volatile fields

Mark a field `Persistence::Volatile` to make it CLI-only — it is never written to the INI file and must be supplied on every run.

```cpp
ConfigLib::ConfigItem::make<std::string>("run_id", std::string(""), "Unique run ID",
    nullptr, ConfigLib::Persistence::Volatile)
```

---

### Validation Rules

Attach a rule as the fourth argument to `ConfigItem::make`:

```cpp
static const ValidationRules::BetweenValues between1And65535(1, 65535);

ConfigLib::ConfigItem::make<int>("port", 8080, "Server port", &between1And65535)
```

Built-in singletons (use directly by address): `greaterThanZero`, `greaterThanOrEqualToZero`.
Parameterized classes (instantiate and keep alive for the lifetime of the config): `BetweenValues(min, max)`, `InList(vector<string>)`. Custom rules implement the `Rule` interface.

### Custom Types

Inherit from `ConfigLib::ConfigType<T>` and implement three methods:

```cpp
struct Color : public ConfigLib::ConfigType<Color>
{
    int r, g, b;
    static const char* typeName() { return "Color"; }
    std::string toString() const { return std::to_string(r)+","+std::to_string(g)+","+std::to_string(b); }
    static Color fromString(const std::string& s) { /* parse r,g,b */ }
};
```

Registration is automatic via the CRTP base — use `Color` in `ConfigItem::make<Color>()` like any built-in type.

## Project Structure

```
cpp-config-ini/
├── source/
│   ├── config_library/
│   │   ├── common/          # ConfigSchema, TypeParser, ValidationRules, SchemaEvolver, Logger
│   │   └── config_reader/   # ConfigReader, CLIFeatureLayer, INI/CLI persistence
│   └── main.cpp             # Main demo
├── examples/                # Standalone example programs (each .cpp is its own executable)
├── tests/                   # One .cpp per test suite, custom lightweight framework
├── CMakeLists.txt
├── configure                # Dependency setup script
├── make                     # Build script (wraps CMake + Ninja)
└── run                      # Run script for the main demo
```

## Examples

The `examples/` directory contains standalone programs that are built automatically with `./make`. Each `.cpp` file compiles to its own executable. See `examples/cli_and_ini/color_migration.cpp` for a full schema migration walkthrough.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
