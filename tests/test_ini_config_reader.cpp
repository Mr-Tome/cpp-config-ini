#include "test_framework.hpp"
#include "config_library/config_reader/config_reader.hpp"
#include <fstream>
#include <cstdio>

// ─── Basic INI config ───────────────────────────────────────────────────────

static const std::string kBasicPath = "configlib_test_basic.ini";

class BasicConfig : public ConfigLib::ConfigReader<BasicConfig, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        using Item = ConfigLib::ConfigItem;
        return {{
            "test",
            {
                Item::make<int>("count",          10,                  "item count"),
                Item::make<double>("ratio",        0.5,                 "a ratio"),
                Item::make<bool>("flag",           false,               "a flag"),
                Item::make<std::string>("name",    std::string("hello"), "a name"),
            }
        }};
    }
    std::string getConfigFilePath() const { return kBasicPath; }
};

static bool test_ini_creates_file_on_first_run()
{
    TempFile guard(kBasicPath);
    {
        SuppressStdout s;
        BasicConfig cfg;
        (void)cfg;
    }
    std::ifstream f(kBasicPath);
    REQUIRE(f.is_open() == true);
    return true;
}

static bool test_getValue_returns_defaults()
{
    TempFile guard(kBasicPath);
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("test", "count"),           10);
    REQUIRE_EQ(cfg.getValue<bool>("test", "flag"),           false);
    REQUIRE_EQ(cfg.getValue<std::string>("test", "name"),    std::string("hello"));
    return true;
}

static bool test_setValue_then_getValue()
{
    TempFile guard(kBasicPath);
    SuppressStdout s;
    BasicConfig cfg;
    cfg.setValue<int>("test", "count", 99);
    REQUIRE_EQ(cfg.getValue<int>("test", "count"), 99);
    return true;
}

static bool test_saveConfig_persists_across_instances()
{
    TempFile guard(kBasicPath);
    {
        SuppressStdout s;
        BasicConfig cfg;
        cfg.setValue<int>("test", "count", 77);
        cfg.saveConfig();
    }
    {
        SuppressStdout s;
        BasicConfig cfg2;
        REQUIRE_EQ(cfg2.getValue<int>("test", "count"), 77);
    }
    return true;
}

static bool test_getValue_wrong_section_throws()
{
    TempFile guard(kBasicPath);
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_THROWS(cfg.getValue<int>("nonexistent_section", "count"));
    return true;
}

static bool test_getValue_wrong_key_throws()
{
    TempFile guard(kBasicPath);
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_THROWS(cfg.getValue<int>("test", "no_such_key"));
    return true;
}

static bool test_getSections_contains_expected_section()
{
    TempFile guard(kBasicPath);
    SuppressStdout s;
    BasicConfig cfg;
    const auto& sects = cfg.getSections();
    REQUIRE(sects.count("test") > static_cast<std::size_t>(0));
    return true;
}

// ─── Schema evolution: new key added ────────────────────────────────────────

static const std::string kEvolvedPath = "configlib_test_evolved.ini";

class SchemaV1Config : public ConfigLib::ConfigReader<SchemaV1Config, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "S", { ConfigLib::ConfigItem::make<int>("old_key", 1, "old") } }};
    }
    std::string getConfigFilePath() const { return kEvolvedPath; }
};

class SchemaV2Config : public ConfigLib::ConfigReader<SchemaV2Config, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "S", {
            ConfigLib::ConfigItem::make<int>("old_key", 1, "old"),
            ConfigLib::ConfigItem::make<int>("new_key", 42, "new"),
        }}};
    }
    std::string getConfigFilePath() const { return kEvolvedPath; }
};

static bool test_schema_evolution_adds_new_key_with_default()
{
    TempFile guard(kEvolvedPath);
    {
        SuppressStdout s;
        SchemaV1Config v1;
        (void)v1;
    }
    {
        SuppressStdout s;
        SchemaV2Config v2;
        REQUIRE_EQ(v2.getValue<int>("S", "new_key"), 42);
        REQUIRE_EQ(v2.getValue<int>("S", "old_key"), 1);
    }
    return true;
}

// ─── Schema migration: rename key across sections ────────────────────────────

static const std::string kMigrationPath = "configlib_test_migration.ini";

class MigratedConfig : public ConfigLib::ConfigReader<MigratedConfig, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "NewSection", {
            ConfigLib::ConfigItem::make<int>("new_key", 0, "migrated key"),
        }}};
    }
    std::string getConfigFilePath() const { return kMigrationPath; }
    uint32_t getSchemaVersion() const { return 2; }
    std::vector<ConfigLib::SchemaMigration> getMigrations() const
    {
        return {
            ConfigLib::Migration::rename(1, 2,
                "OldSection", "old_key",
                "NewSection",  "new_key"),
        };
    }
};

static bool test_migration_renames_key_and_preserves_value()
{
    TempFile guard(kMigrationPath);
    // Write a V1 config file manually
    {
        std::ofstream f(kMigrationPath);
        f << "# __schema_version__ = 1\n\n"
          << "# Configuration file\n\n"
          << "[OldSection]\n"
          << "old_key = 100 # type: int, description: old key\n\n";
    }
    {
        SuppressStdout s;
        MigratedConfig cfg;
        REQUIRE_EQ(cfg.getValue<int>("NewSection", "new_key"), 100);
    }
    return true;
}

// ─── Schema migration: transform value in place ──────────────────────────────

static const std::string kTransformPath = "configlib_test_transform.ini";

class TransformConfig : public ConfigLib::ConfigReader<TransformConfig, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "S", { ConfigLib::ConfigItem::make<int>("val", 0, "a value") } }};
    }
    std::string getConfigFilePath() const { return kTransformPath; }
    uint32_t getSchemaVersion() const { return 2; }
    std::vector<ConfigLib::SchemaMigration> getMigrations() const
    {
        return {
            ConfigLib::Migration::transformInPlace(1, 2, "S", "val",
                [](const std::string& v)
                {
                    return std::to_string(std::stoi(v) * 10);
                }),
        };
    }
};

static bool test_migration_transforms_value_in_place()
{
    TempFile guard(kTransformPath);
    {
        std::ofstream f(kTransformPath);
        f << "# __schema_version__ = 1\n\n"
          << "# Configuration file\n\n"
          << "[S]\n"
          << "val = 7 # type: int, description: a value\n\n";
    }
    {
        SuppressStdout s;
        TransformConfig cfg;
        REQUIRE_EQ(cfg.getValue<int>("S", "val"), 70);
    }
    return true;
}

// ─── Persistence: delete ─────────────────────────────────────────────────────

static bool test_persistDelete_removes_file()
{
    TempFile guard(kBasicPath);
    {
        SuppressStdout s;
        BasicConfig cfg;
        cfg.persistDelete();
    }
    std::ifstream f(kBasicPath);
    REQUIRE(f.is_open() == false);
    return true;
}

static bool test_persistDelete_nonexistent_throws()
{
    TempFile guard(kBasicPath);
    SuppressStdout s;
    BasicConfig cfg;
    cfg.persistDelete();
    REQUIRE_THROWS(cfg.persistDelete());
    return true;
}

// ─── Persistence: export ─────────────────────────────────────────────────────

static const std::string kExportPath = "configlib_test_export.ini";

static bool test_persistExport_writes_to_path()
{
    TempFile guard1(kBasicPath);
    TempFile guard2(kExportPath);
    SuppressStdout s;
    BasicConfig cfg;
    cfg.setValue<int>("test", "count", 77);
    auto sections = ConfigLib::mergeDuplicateSections(cfg.getConfigSections());
    cfg.persistExport(kExportPath, sections);
    std::ifstream f(kExportPath);
    REQUIRE(f.is_open() == true);
    std::string content(
        (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    REQUIRE(content.find("77") != std::string::npos);
    return true;
}

// ─── Orphaned key policy: Remove ─────────────────────────────────────────────

static const std::string kRemovePolicyPath = "configlib_test_remove_policy.ini";

class RemovePolicyConfig : public ConfigLib::ConfigReader<RemovePolicyConfig, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "S", { ConfigLib::ConfigItem::make<int>("key", 1, "a key") } }};
    }
    std::string getConfigFilePath() const { return kRemovePolicyPath; }
    ConfigLib::OrphanedConfigItemPolicy getOrphanedConfigItemPolicy() const
    {
        return ConfigLib::OrphanedConfigItemPolicy::Remove;
    }
};

static bool test_orphanedPolicy_Remove_strips_key_from_file()
{
    TempFile guard(kRemovePolicyPath);
    {
        std::ofstream f(kRemovePolicyPath);
        f << "[S]\n"
          << "key = 5 # type: int, description: a key\n"
          << "orphaned = 99 # type: int, description: old key\n\n";
    }
    {
        SuppressStdout s;
        RemovePolicyConfig cfg;
        REQUIRE_EQ(cfg.getValue<int>("S", "key"), 5);
    }
    std::ifstream fin(kRemovePolicyPath);
    std::string content(
        (std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
    REQUIRE(content.find("orphaned") == std::string::npos);
    return true;
}

int main()
{
    return runTests({
        {"INIConfigReader: creates file on first run",              test_ini_creates_file_on_first_run},
        {"INIConfigReader: getValue returns defaults",              test_getValue_returns_defaults},
        {"INIConfigReader: setValue then getValue",                 test_setValue_then_getValue},
        {"INIConfigReader: saveConfig persists across instances",   test_saveConfig_persists_across_instances},
        {"INIConfigReader: getValue wrong section throws",          test_getValue_wrong_section_throws},
        {"INIConfigReader: getValue wrong key throws",              test_getValue_wrong_key_throws},
        {"INIConfigReader: getSections contains expected section",  test_getSections_contains_expected_section},
        {"Schema evolution: new key added with default",           test_schema_evolution_adds_new_key_with_default},
        {"Schema migration: rename preserves value",               test_migration_renames_key_and_preserves_value},
        {"Schema migration: transform in place",                   test_migration_transforms_value_in_place},
        {"persistDelete: removes file",                            test_persistDelete_removes_file},
        {"persistDelete: nonexistent throws",                      test_persistDelete_nonexistent_throws},
        {"persistExport: writes to new path",                      test_persistExport_writes_to_path},
        {"OrphanedPolicy::Remove: strips orphaned key from file",  test_orphanedPolicy_Remove_strips_key_from_file},
    });
}
