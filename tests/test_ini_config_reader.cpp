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

// ─── INI file format resilience ──────────────────────────────────────────────

static bool test_ini_loads_value_with_inline_comment()
{
    TempFile guard(kBasicPath);
    {
        std::ofstream f(kBasicPath);
        f << "[test]\n"
          << "count = 42 # user's note here\n"
          << "flag = true # another note\n"
          << "ratio = 0.5\n"
          << "name = hello\n\n";
    }
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("test", "count"), 42);
    REQUIRE_EQ(cfg.getValue<bool>("test", "flag"), true);
    return true;
}

static bool test_ini_loads_value_with_extra_whitespace_around_equals()
{
    TempFile guard(kBasicPath);
    {
        std::ofstream f(kBasicPath);
        f << "[test]\n"
          << "count  =  99  \n"
          << "flag  =  false  \n"
          << "ratio  =  1.5  \n"
          << "name  =  world  \n\n";
    }
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("test", "count"), 99);
    REQUIRE_EQ(cfg.getValue<std::string>("test", "name"), std::string("world"));
    return true;
}

static bool test_empty_ini_file_triggers_evolution_with_defaults()
{
    TempFile guard(kBasicPath);
    { std::ofstream f(kBasicPath); } // create empty file
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("test", "count"), 10);
    REQUIRE_EQ(cfg.getValue<bool>("test", "flag"), false);
    return true;
}

static bool test_ini_type_mismatch_in_file_falls_back_to_default()
{
    TempFile guard(kBasicPath);
    {
        std::ofstream f(kBasicPath);
        f << "[test]\n"
          << "count = not_a_number # type: int, description: item count\n"
          << "flag = false\nratio = 0.5\nname = hello\n\n";
    }
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("test", "count"), 10); // default restored
    return true;
}

static bool test_comment_out_policy_writes_deprecated_comment_in_file()
{
    TempFile guard(kBasicPath);
    {
        std::ofstream f(kBasicPath);
        f << "[test]\n"
          << "count = 10\nflag = false\nratio = 0.5\nname = hello\n"
          << "orphaned_key = 99 # old key\n\n";
    }
    {
        SuppressStdout s;
        BasicConfig cfg;
        (void)cfg;
    }
    std::ifstream fin(kBasicPath);
    std::string content((std::istreambuf_iterator<char>(fin)),
                         std::istreambuf_iterator<char>());
    REQUIRE(content.find("[deprecated]")  != std::string::npos);
    REQUIRE(content.find("orphaned_key")  != std::string::npos);
    return true;
}

// ─── Volatile field in INI-only config ───────────────────────────────────────

class VolatileINIOnlyConfig
    : public ConfigLib::ConfigReader<VolatileINIOnlyConfig, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "run", {
            ConfigLib::ConfigItem::make<std::string>(
                "id", std::string(""), "run id",
                nullptr, ConfigLib::Persistence::Volatile),
        }}};
    }
    std::string getConfigFilePath() const
    {
        return "configlib_test_volatile_ini_only.ini";
    }
};

static bool test_volatile_field_in_ini_only_config_throws()
{
    REQUIRE_THROWS(VolatileINIOnlyConfig());
    return true;
}

// ─── setValue validation failure ─────────────────────────────────────────────

static const std::string kValidationConfigPath = "configlib_test_valrule.ini";

class ValidatedINIConfig
    : public ConfigLib::ConfigReader<ValidatedINIConfig, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "cfg", {
            ConfigLib::ConfigItem::make<int>(
                "count", 5, "count", &ValidationRules::greaterThanZero),
        }}};
    }
    std::string getConfigFilePath() const { return kValidationConfigPath; }
};

static bool test_setValue_validation_failure_throws()
{
    TempFile guard(kValidationConfigPath);
    SuppressStdout s;
    ValidatedINIConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("cfg", "count"), 5);
    REQUIRE_THROWS(cfg.setValue<int>("cfg", "count", -1));
    REQUIRE_EQ(cfg.getValue<int>("cfg", "count"), 5); // unchanged
    return true;
}

// ─── Multi-hop migration (v1 → v3 via two steps) ─────────────────────────────

static const std::string kMultiHopPath = "configlib_test_multihop.ini";

class MultiHopConfig
    : public ConfigLib::ConfigReader<MultiHopConfig, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "S", { ConfigLib::ConfigItem::make<int>("val", 0, "value") } }};
    }
    std::string getConfigFilePath() const { return kMultiHopPath; }
    uint32_t getSchemaVersion() const { return 3; }
    std::vector<ConfigLib::SchemaMigration> getMigrations() const
    {
        return {
            ConfigLib::Migration::rename(1, 2, "S", "old_val", "S", "mid_val"),
            ConfigLib::Migration::rename(2, 3, "S", "mid_val", "S", "val"),
        };
    }
};

static bool test_multihop_migration_applies_both_steps()
{
    TempFile guard(kMultiHopPath);
    {
        std::ofstream f(kMultiHopPath);
        f << "# __schema_version__ = 1\n\n"
          << "[S]\n"
          << "old_val = 42 # type: int, description: value\n\n";
    }
    {
        SuppressStdout s;
        MultiHopConfig cfg;
        REQUIRE_EQ(cfg.getValue<int>("S", "val"), 42);
    }
    return true;
}

// ─── Schema evolution file annotation ────────────────────────────────────────

static bool test_schema_evolution_writes_added_key_annotation()
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
        (void)v2;
    }
    std::ifstream fin(kEvolvedPath);
    std::string content((std::istreambuf_iterator<char>(fin)),
                         std::istreambuf_iterator<char>());
    REQUIRE(content.find("added by schema update") != std::string::npos);
    REQUIRE(content.find("new_key")                != std::string::npos);
    return true;
}

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

// ─── setValue: wrong section throws ─────────────────────────────────────────

static bool test_setValue_wrong_section_throws()
{
    TempFile guard(kBasicPath);
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_THROWS(cfg.setValue<int>("nonexistent_section", "count", 1));
    return true;
}

// ─── Multi-section config ────────────────────────────────────────────────────

static const std::string kMultiSectionPath = "configlib_test_multisection.ini";

class MultiSectionConfig : public ConfigLib::ConfigReader<MultiSectionConfig, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        using Item = ConfigLib::ConfigItem;
        return {
            { "sectionA", { Item::make<int>("x", 10, "x value") } },
            { "sectionB", { Item::make<std::string>("y", std::string("hello"), "y value") } },
        };
    }
    std::string getConfigFilePath() const { return kMultiSectionPath; }
};

static bool test_multi_section_defaults_and_set()
{
    TempFile guard(kMultiSectionPath);
    SuppressStdout s;
    MultiSectionConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("sectionA", "x"), 10);
    REQUIRE_EQ(cfg.getValue<std::string>("sectionB", "y"), std::string("hello"));
    cfg.setValue<int>("sectionA", "x", 99);
    REQUIRE_EQ(cfg.getValue<int>("sectionA", "x"), 99);
    const auto& sects = cfg.getSections();
    REQUIRE(sects.count("sectionA") > static_cast<std::size_t>(0));
    REQUIRE(sects.count("sectionB") > static_cast<std::size_t>(0));
    return true;
}

// ─── vector<int> persists across instances ───────────────────────────────────

static const std::string kVecIntPath = "configlib_test_vecint.ini";

class VecIntConfig : public ConfigLib::ConfigReader<VecIntConfig, ConfigLib::INI>
{
public:
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{ "data", {
            ConfigLib::ConfigItem::make<std::vector<int>>(
                "nums", std::vector<int>{1, 2, 3}, "a list"),
        }}};
    }
    std::string getConfigFilePath() const { return kVecIntPath; }
};

static bool test_vector_int_persists_across_instances()
{
    TempFile guard(kVecIntPath);
    {
        SuppressStdout s;
        VecIntConfig cfg;
        cfg.setValue<std::vector<int>>("data", "nums", std::vector<int>{4, 5, 6});
        cfg.saveConfig();
    }
    {
        SuppressStdout s;
        VecIntConfig cfg;
        auto v = cfg.getValue<std::vector<int>>("data", "nums");
        REQUIRE_EQ(static_cast<int>(v.size()), 3);
        REQUIRE_EQ(v[0], 4);
        REQUIRE_EQ(v[2], 6);
    }
    return true;
}

static bool test_ini_key_before_section_header_is_ignored()
{
    TempFile guard(kBasicPath);
    {
        std::ofstream f(kBasicPath);
        f << "orphan_before_section = ignored\n"
          << "[test]\n"
          << "count = 42\nflag = false\nratio = 0.5\nname = hello\n\n";
    }
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("test", "count"), 42);
    return true;
}

static bool test_ini_value_with_equals_sign_is_preserved()
{
    TempFile guard(kBasicPath);
    {
        std::ofstream f(kBasicPath);
        f << "[test]\n"
          << "count = 10\nflag = false\nratio = 0.5\n"
          << "name = key=value\n\n";
    }
    SuppressStdout s;
    BasicConfig cfg;
    REQUIRE_EQ(cfg.getValue<std::string>("test", "name"), std::string("key=value"));
    return true;
}

static bool test_ini_validation_failure_in_file_falls_back_to_default()
{
    TempFile guard(kValidationConfigPath);
    {
        std::ofstream f(kValidationConfigPath);
        f << "[cfg]\n"
          << "count = -5 # type: int, description: count\n\n";
    }
    SuppressStdout s;
    ValidatedINIConfig cfg;
    REQUIRE_EQ(cfg.getValue<int>("cfg", "count"), 5);
    return true;
}

int main()
{
    return runTests({
        {"INIConfigReader: creates file on first run",                test_ini_creates_file_on_first_run},
        {"INIConfigReader: getValue returns defaults",                test_getValue_returns_defaults},
        {"INIConfigReader: setValue then getValue",                   test_setValue_then_getValue},
        {"INIConfigReader: setValue wrong section throws",            test_setValue_wrong_section_throws},
        {"INIConfigReader: saveConfig persists across instances",     test_saveConfig_persists_across_instances},
        {"INIConfigReader: getValue wrong section throws",            test_getValue_wrong_section_throws},
        {"INIConfigReader: getValue wrong key throws",                test_getValue_wrong_key_throws},
        {"INIConfigReader: getSections contains expected section",    test_getSections_contains_expected_section},
        {"INIConfigReader: multi-section defaults and set",           test_multi_section_defaults_and_set},
        {"INIConfigReader: vector<int> persists across instances",    test_vector_int_persists_across_instances},
        {"INI format: inline comment is stripped",                    test_ini_loads_value_with_inline_comment},
        {"INI format: extra whitespace around = is trimmed",         test_ini_loads_value_with_extra_whitespace_around_equals},
        {"INI format: empty file triggers evolution with defaults",   test_empty_ini_file_triggers_evolution_with_defaults},
        {"INI format: type mismatch falls back to default",          test_ini_type_mismatch_in_file_falls_back_to_default},
        {"INI format: CommentOut writes # [deprecated] in file",     test_comment_out_policy_writes_deprecated_comment_in_file},
        {"INIConfigReader: volatile field in INI-only throws",        test_volatile_field_in_ini_only_config_throws},
        {"INIConfigReader: setValue validation failure throws",       test_setValue_validation_failure_throws},
        {"Schema evolution: new key added with default",             test_schema_evolution_adds_new_key_with_default},
        {"Schema evolution: annotation written to file",             test_schema_evolution_writes_added_key_annotation},
        {"Schema migration: rename preserves value",                 test_migration_renames_key_and_preserves_value},
        {"Schema migration: transform in place",                     test_migration_transforms_value_in_place},
        {"Schema migration: multi-hop v1 to v3",                     test_multihop_migration_applies_both_steps},
        {"persistDelete: removes file",                              test_persistDelete_removes_file},
        {"persistDelete: nonexistent throws",                        test_persistDelete_nonexistent_throws},
        {"persistExport: writes to new path",                        test_persistExport_writes_to_path},
        {"OrphanedPolicy::Remove: strips orphaned key from file",    test_orphanedPolicy_Remove_strips_key_from_file},
        {"INI format: key before section header is ignored",          test_ini_key_before_section_header_is_ignored},
        {"INI format: value containing = sign is preserved",          test_ini_value_with_equals_sign_is_preserved},
        {"INI format: validation failure in file uses default",       test_ini_validation_failure_in_file_falls_back_to_default},
    });
}
