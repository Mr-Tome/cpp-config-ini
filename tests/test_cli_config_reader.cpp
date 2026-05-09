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

// ─── --print flag ────────────────────────────────────────────────────────────

static bool test_cli_print_flag_outputs_current_values()
{
    std::vector<std::string> args = {"prog", "--settings.threads=8", "--print"};
    auto argv = makeArgv(args);

    std::ostringstream captured;
    std::streambuf* oldBuf = std::cout.rdbuf(captured.rdbuf());
    CLIOnlyConfig cfg(static_cast<int>(args.size()), argv.data());
    std::cout.rdbuf(oldBuf);

    const std::string out = captured.str();
    REQUIRE(out.find("threads") != std::string::npos);
    REQUIRE(out.find("8")       != std::string::npos);
    REQUIRE_EQ(cfg.getValue<int>("settings", "threads"), 8);
    return true;
}

// ─── --diff flag ──────────────────────────────────────────────────────────────

static bool test_cli_diff_flag_shows_changed_value()
{
    std::vector<std::string> args = {"prog", "--settings.threads=99", "--diff"};
    auto argv = makeArgv(args);

    std::ostringstream captured;
    std::streambuf* oldBuf = std::cout.rdbuf(captured.rdbuf());
    CLIOnlyConfig cfg(static_cast<int>(args.size()), argv.data());
    std::cout.rdbuf(oldBuf);

    const std::string out = captured.str();
    REQUIRE(out.find("threads") != std::string::npos);
    REQUIRE(out.find("99")      != std::string::npos);
    return true;
}

static bool test_cli_diff_flag_no_changes_says_so()
{
    std::vector<std::string> args = {"prog", "--diff"};
    auto argv = makeArgv(args);

    std::ostringstream captured;
    std::streambuf* oldBuf = std::cout.rdbuf(captured.rdbuf());
    CLIOnlyConfig cfg(static_cast<int>(args.size()), argv.data());
    std::cout.rdbuf(oldBuf);

    const std::string out = captured.str();
    REQUIRE(out.find("all values match schema defaults") != std::string::npos);
    return true;
}

// ─── --save flag ──────────────────────────────────────────────────────────────

static bool test_inicli_save_flag_writes_cli_override_to_file()
{
    TempFile guard(kINICLIPath);
    {
        SuppressStdout s;
        INICLIConfig base;
        (void)base;
    }
    {
        std::vector<std::string> args = {"prog", "--app.level=7", "--save"};
        auto argv = makeArgv(args);
        SuppressStdout s;
        INICLIConfig cfg(static_cast<int>(args.size()), argv.data());
        REQUIRE_EQ(cfg.getValue<int>("app", "level"), 7);
    }
    {
        SuppressStdout s;
        INICLIConfig reloaded;
        REQUIRE_EQ(reloaded.getValue<int>("app", "level"), 7);
    }
    return true;
}

// ─── --flat= runtime override ─────────────────────────────────────────────────

static bool test_cli_flat_runtime_false_disables_flat_lookup()
{
    std::vector<std::string> args = {"prog", "--flat=false", "--threads=8"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(CLIOnlyConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

static bool test_cli_flat_runtime_false_qualified_key_still_works()
{
    std::vector<std::string> args = {"prog", "--flat=false", "--settings.threads=8"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    CLIOnlyConfig cfg(static_cast<int>(args.size()), argv.data());
    REQUIRE_EQ(cfg.getValue<int>("settings", "threads"), 8);
    return true;
}

// ─── Ambiguous flat key ───────────────────────────────────────────────────────

class AmbiguousKeyConfig : public ConfigLib::ConfigReader<AmbiguousKeyConfig, ConfigLib::CLI>
{
    using Base = ConfigLib::ConfigReader<AmbiguousKeyConfig, ConfigLib::CLI>;
public:
    using Base::Base;
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {
            { "A", { ConfigLib::ConfigItem::make<int>("x", 0, "x in A") } },
            { "B", { ConfigLib::ConfigItem::make<int>("x", 0, "x in B") } },
        };
    }
};

// ─── Extra CLI edge cases ─────────────────────────────────────────────────────

static bool test_cli_arg_without_double_dash_throws()
{
    std::vector<std::string> args = {"prog", "notanoption"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(CLIOnlyConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

static bool test_cli_flat_invalid_value_throws()
{
    std::vector<std::string> args = {"prog", "--flat=maybe"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(CLIOnlyConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

static bool test_cli_flat_empty_value_throws()
{
    std::vector<std::string> args = {"prog", "--flat="};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(CLIOnlyConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

static bool test_cli_config_empty_path_throws()
{
    std::vector<std::string> args = {"prog", "--config="};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(INICLIConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

static const std::string kCLIExportPath = "configlib_test_cli_export.ini";

static bool test_inicli_export_flag_writes_file()
{
    TempFile guard1(kINICLIPath);
    TempFile guard2(kCLIExportPath);
    {
        SuppressStdout s;
        INICLIConfig base;
        (void)base;
    }
    {
        std::string exportArg = "--export=" + kCLIExportPath;
        std::vector<std::string> args = {"prog", exportArg};
        auto argv = makeArgv(args);
        SuppressStdout s;
        INICLIConfig cfg(static_cast<int>(args.size()), argv.data());
        (void)cfg;
    }
    std::ifstream f(kCLIExportPath);
    REQUIRE(f.is_open() == true);
    return true;
}

static bool test_cli_export_empty_path_throws()
{
    TempFile guard(kINICLIPath);
    {
        SuppressStdout s;
        INICLIConfig base;
        (void)base;
    }
    std::vector<std::string> args = {"prog", "--export="};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(INICLIConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

static bool test_cli_ambiguous_flat_key_throws()
{
    std::vector<std::string> args = {"prog", "--x=5"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    REQUIRE_THROWS(AmbiguousKeyConfig(static_cast<int>(args.size()), argv.data()));
    return true;
}

static bool test_cli_ambiguous_key_qualified_form_works()
{
    std::vector<std::string> args = {"prog", "--A.x=5"};
    auto argv = makeArgv(args);
    SuppressStdout s;
    AmbiguousKeyConfig cfg(static_cast<int>(args.size()), argv.data());
    REQUIRE_EQ(cfg.getValue<int>("A", "x"), 5);
    REQUIRE_EQ(cfg.getValue<int>("B", "x"), 0);
    return true;
}

int main()
{
    return runTests({
        {"CLI: default constructor gets defaults",               test_cli_default_constructor_gets_defaults},
        {"CLI: qualified key override",                         test_cli_qualified_key_override},
        {"CLI: flat key override",                              test_cli_flat_key_override},
        {"CLI: unknown section throws",                         test_cli_unknown_section_throws},
        {"CLI: unknown key throws",                             test_cli_unknown_key_throws},
        {"CLI: volatile field missing throws",                  test_cli_volatile_missing_throws},
        {"CLI: volatile field provided succeeds",               test_cli_volatile_provided_succeeds},
        {"CLI: volatile field value is accessible",             test_cli_volatile_value_is_accessible},
        {"CLI: flattenCLIArgs=false qualified key works",       test_cli_flat_disabled_qualified_key_works},
        {"CLI: flattenCLIArgs=false flat key throws",           test_cli_flat_disabled_flat_key_throws},
        {"CLI: --print outputs current values",                 test_cli_print_flag_outputs_current_values},
        {"CLI: --diff shows changed values",                    test_cli_diff_flag_shows_changed_value},
        {"CLI: --diff with no changes says so",                 test_cli_diff_flag_no_changes_says_so},
        {"CLI: --flat=false disables flat lookup at runtime",   test_cli_flat_runtime_false_disables_flat_lookup},
        {"CLI: --flat=false qualified key still works",         test_cli_flat_runtime_false_qualified_key_still_works},
        {"CLI: arg without -- prefix throws",                   test_cli_arg_without_double_dash_throws},
        {"CLI: --flat= with invalid bool throws",               test_cli_flat_invalid_value_throws},
        {"CLI: --flat= with empty value throws",                test_cli_flat_empty_value_throws},
        {"CLI: ambiguous flat key throws",                      test_cli_ambiguous_flat_key_throws},
        {"CLI: ambiguous key qualified form works",             test_cli_ambiguous_key_qualified_form_works},
        {"INI+CLI: default creates file with default value",    test_inicli_default_creates_file_and_gets_default},
        {"INI+CLI: CLI override beats saved file value",        test_inicli_cli_overrides_saved_value},
        {"INI+CLI: --config= redirects file path",              test_inicli_config_path_override},
        {"INI+CLI: --save writes CLI override to file",         test_inicli_save_flag_writes_cli_override_to_file},
        {"INI+CLI: --config= with empty path throws",           test_cli_config_empty_path_throws},
        {"INI+CLI: --export= writes file",                      test_inicli_export_flag_writes_file},
        {"INI+CLI: --export= with empty path throws",           test_cli_export_empty_path_throws},
    });
}
