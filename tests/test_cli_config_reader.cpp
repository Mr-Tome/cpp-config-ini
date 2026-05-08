#include "test_framework.hpp"
#include "config_library/config_reader/config_reader.hpp"
#include <fstream>
#include <cstdio>

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Build a null-terminated argv from a vector of strings (avoids const_cast)
static std::vector<char*> makeArgv(std::vector<std::string>& args)
{
    std::vector<char*> ptrs;
    ptrs.reserve(args.size() + 1);
    for (auto& s : args) ptrs.push_back(&s[0]);
    ptrs.push_back(nullptr);
    return ptrs;
}

// ─── CLI-only config ──────────────────────────────────────────────────────────

class CLIOnlyConfig : public ConfigLib::ConfigReader<CLIOnlyConfig, ConfigLib::CLI>
{
    using Base = ConfigLib::ConfigReader<CLIOnlyConfig, ConfigLib::CLI>;
public:
    using Base::Base;
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        using Item = ConfigLib::ConfigItem;
        return {{ "settings", {
            Item::make<int>("threads",         4,                    "thread count"),
            Item::make<std::string>("mode",    std::string("fast"),  "run mode"),
        }}};
    }
};

static bool test_cli_default_constructor_gets_defaults()
{
    SuppressStdout s;
    CLIOnlyConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("settings", "threads"),        4);
    REQUIRE_EQ(cfg.getValue<std::string>("settings", "mode"),   std::string("fast"));
    return true;
}

static bool test_cli_qualified_key_override()
{
    std::vector<std::string> args = {"prog", "--settings.threads=8"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    CLIOnlyConfig cfg(static_cast<int>(args.size()), argv.data());
    REQUIRE_EQ(cfg.getValue<int>("settings", "threads"),       8);
    REQUIRE_EQ(cfg.getValue<std::string>("settings", "mode"),  std::string("fast"));
    return true;
}

static bool test_cli_flat_key_override()
{
    std::vector<std::string> args = {"prog", "--threads=12"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    CLIOnlyConfig cfg(static_cast<int>(args.size()), argv.data());
    REQUIRE_EQ(cfg.getValue<int>("settings", "threads"), 12);
    return true;
}

static bool test_cli_unknown_section_throws()
{
    std::vector<std::string> args = {"prog", "--bogus.key=5"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(CLIOnlyConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

static bool test_cli_unknown_key_throws()
{
    std::vector<std::string> args = {"prog", "--settings.no_such=5"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(CLIOnlyConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

// ─── Volatile fields ──────────────────────────────────────────────────────────

class VolatileRunConfig : public ConfigLib::ConfigReader<VolatileRunConfig, ConfigLib::CLI>
{
    using Base = ConfigLib::ConfigReader<VolatileRunConfig, ConfigLib::CLI>;
public:
    using Base::Base;
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "run", {
            ConfigLib::ConfigItem::make<std::string>(
                "id", std::string(""), "run identifier",
                nullptr, ConfigLib::Persistence::Volatile),
        }}};
    }
};

static bool test_cli_volatile_missing_throws()
{
    SuppressStdout s;
    REQUIRE_THROWS(VolatileRunConfig());
    return true;
}

static bool test_cli_volatile_provided_succeeds()
{
    std::vector<std::string> args = {"prog", "--run.id=test-run-001"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_NO_THROW(VolatileRunConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

static bool test_cli_volatile_value_is_accessible()
{
    std::vector<std::string> args = {"prog", "--run.id=my-run-123"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    VolatileRunConfig cfg(static_cast<int>(args.size()), argv.data());
    REQUIRE_EQ(cfg.getValue<std::string>("run", "id"), std::string("my-run-123"));
    return true;
}

// ─── flattenCLIArgs() override ────────────────────────────────────────────────

class NoFlatConfig : public ConfigLib::ConfigReader<NoFlatConfig, ConfigLib::CLI>
{
    using Base = ConfigLib::ConfigReader<NoFlatConfig, ConfigLib::CLI>;
public:
    using Base::Base;
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "cfg", { ConfigLib::ConfigItem::make<int>("x", 0, "a value") } }};
    }
    bool flattenCLIArgs() const { return false; }
};

static bool test_cli_flat_disabled_qualified_key_works()
{
    std::vector<std::string> args = {"prog", "--cfg.x=99"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    NoFlatConfig cfg(static_cast<int>(args.size()), argv.data());
    REQUIRE_EQ(cfg.getValue<int>("cfg", "x"), 99);
    return true;
}

static bool test_cli_flat_disabled_flat_key_throws()
{
    std::vector<std::string> args = {"prog", "--x=99"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(NoFlatConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

// ─── INI + CLI combo ──────────────────────────────────────────────────────────

static const std::string kINICLIPath = "configlib_test_inicli.ini";

class INICLIConfig : public ConfigLib::ConfigReader<INICLIConfig, ConfigLib::INI, ConfigLib::CLI>
{
    using Base = ConfigLib::ConfigReader<INICLIConfig, ConfigLib::INI, ConfigLib::CLI>;
public:
    using Base::Base;
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "app", { ConfigLib::ConfigItem::make<int>("level", 1, "log level") } }};
    }
    std::string getConfigFilePath() const { return kINICLIPath; }
};

static bool test_inicli_default_creates_file_and_gets_default()
{
    TempFile guard(kINICLIPath);
    {
        SuppressStdout s;
        INICLIConfig cfg;
        REQUIRE_EQ(cfg.getValue<int>("app", "level"), 1);
    }
    std::ifstream f(kINICLIPath);
    REQUIRE(f.is_open() == true);
    return true;
}

static bool test_inicli_cli_overrides_saved_value()
{
    TempFile guard(kINICLIPath);
    {
        SuppressStdout s;
        INICLIConfig cfg;
        cfg.setValue<int>("app", "level", 5);
        cfg.saveConfig();
    }
    {
        std::vector<std::string> args = {"prog", "--app.level=99"};
        auto argv = makeArgv(args);
        SuppressStdout s;
        INICLIConfig cfg(static_cast<int>(args.size()), argv.data());
        REQUIRE_EQ(cfg.getValue<int>("app", "level"), 99);
    }
    return true;
}

static bool test_inicli_config_path_override()
{
    static const std::string kAltPath = "configlib_test_alt.ini";
    TempFile guard1(kINICLIPath);
    TempFile guard2(kAltPath);
    {
        std::string configArg = "--config=" + kAltPath;
        std::vector<std::string> args = {"prog", configArg};
        auto argv = makeArgv(args);
        SuppressStdout s;
        INICLIConfig cfg(static_cast<int>(args.size()), argv.data());
        REQUIRE_EQ(cfg.getValue<int>("app", "level"), 1);
    }
    std::ifstream f1(kINICLIPath);
    REQUIRE(f1.is_open() == false);
    std::ifstream f2(kAltPath);
    REQUIRE(f2.is_open() == true);
    return true;
}

int main()
{
    return runTests({
        {"CLI: default constructor gets defaults",              test_cli_default_constructor_gets_defaults},
        {"CLI: qualified key override",                        test_cli_qualified_key_override},
        {"CLI: flat key override",                             test_cli_flat_key_override},
        {"CLI: unknown section throws",                        test_cli_unknown_section_throws},
        {"CLI: unknown key throws",                            test_cli_unknown_key_throws},
        {"CLI: volatile field missing throws",                 test_cli_volatile_missing_throws},
        {"CLI: volatile field provided succeeds",              test_cli_volatile_provided_succeeds},
        {"CLI: volatile field value is accessible",            test_cli_volatile_value_is_accessible},
        {"CLI: flattenCLIArgs=false qualified key works",      test_cli_flat_disabled_qualified_key_works},
        {"CLI: flattenCLIArgs=false flat key throws",          test_cli_flat_disabled_flat_key_throws},
        {"INI+CLI: default creates file with default value",   test_inicli_default_creates_file_and_gets_default},
        {"INI+CLI: CLI override beats saved file value",       test_inicli_cli_overrides_saved_value},
        {"INI+CLI: --config= redirects file path",             test_inicli_config_path_override},
    });
}
