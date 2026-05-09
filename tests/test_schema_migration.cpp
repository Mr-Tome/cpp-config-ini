#include "test_framework.hpp"
#include "config_library/common/schema_migration.hpp"
#include <fstream>
#include <cstdio>
#include <sstream>

using ConfigLib::RawConfigMap;
using ConfigLib::SchemaMigration;
using ConfigLib::SectionScopedMigration;
using ConfigLib::Migration::applyMigrations;
using ConfigLib::Migration::rename;
using ConfigLib::Migration::renameKey;
using ConfigLib::Migration::renameSection;
using ConfigLib::Migration::transformInPlace;
using ConfigLib::Migration::transformKey;
using ConfigLib::Migration::parseSchemaVersion;
using ConfigLib::Migration::logMigrationResult;
using ConfigLib::Migration::invalidSchemaVersion;

static bool test_applyMigrations_none_returns_unchanged()
{
    RawConfigMap raw;
    raw["S"]["key"] = "value";
    auto pair = applyMigrations(raw, {}, 1, 1);
    REQUIRE_EQ(pair.first["S"]["key"], std::string("value"));
    REQUIRE(pair.second.ranAny() == false);
    return true;
}

static bool test_rename_moves_value()
{
    RawConfigMap raw;
    raw["OldSection"]["oldKey"] = "42";

    auto m   = rename(1, 2, "OldSection", "oldKey", "NewSection", "newKey");
    auto pair = applyMigrations(raw, {m}, 1, 2);
    const RawConfigMap& result = pair.first;
    const auto& info           = pair.second;

    REQUIRE(result.count("OldSection") == static_cast<std::size_t>(0));
    REQUIRE_EQ(result.at("NewSection").at("newKey"), std::string("42"));
    REQUIRE(info.ranAny() == true);
    REQUIRE_EQ(static_cast<int>(info.changes.size()), 1);
    REQUIRE_EQ(info.changes[0].oldSection, std::string("OldSection"));
    REQUIRE_EQ(info.changes[0].newSection, std::string("NewSection"));
    REQUIRE_EQ(info.changes[0].oldValue,   std::string("42"));
    return true;
}

static bool test_rename_with_transform()
{
    RawConfigMap raw;
    raw["S"]["k"] = "10";

    auto m    = rename(1, 2, "S", "k", "S", "k",
                       [](const std::string& v) { return v + "00"; });
    auto pair  = applyMigrations(raw, {m}, 1, 2);
    const RawConfigMap& result = pair.first;
    const auto& info           = pair.second;

    REQUIRE_EQ(result.at("S").at("k"), std::string("1000"));
    REQUIRE_EQ(info.changes[0].oldValue, std::string("10"));
    REQUIRE_EQ(info.changes[0].newValue, std::string("1000"));
    return true;
}

static bool test_transformInPlace_applies_in_place()
{
    RawConfigMap raw;
    raw["S"]["val"] = "5";

    auto m    = transformInPlace(1, 2, "S", "val",
                    [](const std::string& v)
                    { return std::to_string(std::stoi(v) * 2); });
    auto pair  = applyMigrations(raw, {m}, 1, 2);

    REQUIRE_EQ(pair.first.at("S").at("val"), std::string("10"));
    return true;
}

static bool test_transformInPlace_null_fn_throws()
{
    REQUIRE_THROWS(transformInPlace(1, 2, "S", "k", nullptr));
    return true;
}

static bool test_transformKey_null_fn_throws()
{
    REQUIRE_THROWS(transformKey("k", nullptr));
    return true;
}

static bool test_renameSection_moves_all_keys()
{
    RawConfigMap raw;
    raw["Physics"]["mass"]  = "1.0";
    raw["Physics"]["speed"] = "5.0";

    auto m    = renameSection(1, 2, "Physics", "Dynamics");
    auto pair  = applyMigrations(raw, {m}, 1, 2);
    const RawConfigMap& result = pair.first;
    const auto& info           = pair.second;

    REQUIRE(result.count("Physics") == static_cast<std::size_t>(0));
    REQUIRE_EQ(result.at("Dynamics").at("mass"),  std::string("1.0"));
    REQUIRE_EQ(result.at("Dynamics").at("speed"), std::string("5.0"));
    REQUIRE_EQ(static_cast<int>(info.changes.size()), 2);
    return true;
}

static bool test_renameSection_with_renameKey_child()
{
    RawConfigMap raw;
    raw["Old"]["dt"]   = "0.01";
    raw["Old"]["mass"] = "2.0";

    auto m = renameSection(1, 2, "Old", "New", {
        renameKey("dt", "timestep"),
    });
    auto pair  = applyMigrations(raw, {m}, 1, 2);
    const RawConfigMap& result = pair.first;

    REQUIRE_EQ(result.at("New").at("timestep"), std::string("0.01"));
    REQUIRE_EQ(result.at("New").at("mass"),     std::string("2.0"));
    REQUIRE(result.at("New").count("dt") == static_cast<std::size_t>(0));
    return true;
}

static bool test_renameSection_with_transformKey_child()
{
    RawConfigMap raw;
    raw["Old"]["val"] = "10";

    auto m = renameSection(1, 2, "Old", "New", {
        transformKey("val", [](const std::string& v)
        {
            return std::to_string(std::stoi(v) + 5);
        }),
    });
    auto pair = applyMigrations(raw, {m}, 1, 2);
    REQUIRE_EQ(pair.first.at("New").at("val"), std::string("15"));
    return true;
}

static bool test_renameKey_with_transform()
{
    // renameKey() is used as a child of renameSection() across different sections
    RawConfigMap raw;
    raw["Src"]["old_key"] = "100";
    raw["Src"]["other"]   = "99";

    auto child = renameKey("old_key", "new_key",
                           [](const std::string& v)
                           { return std::to_string(std::stoi(v) / 10); });
    auto m = renameSection(1, 2, "Src", "Dst", {child});
    auto pair = applyMigrations(raw, {m}, 1, 2);
    const RawConfigMap& result = pair.first;

    REQUIRE_EQ(result.at("Dst").at("new_key"), std::string("10"));
    REQUIRE_EQ(result.at("Dst").at("other"),   std::string("99"));
    REQUIRE(result.count("Src") == static_cast<std::size_t>(0));
    REQUIRE(result.at("Dst").count("old_key") == static_cast<std::size_t>(0));
    return true;
}

static bool test_migration_out_of_version_range_not_applied()
{
    RawConfigMap raw;
    raw["S"]["k"] = "1";

    auto m    = rename(3, 4, "S", "k", "S", "moved");
    auto pair  = applyMigrations(raw, {m}, 1, 2);

    REQUIRE(pair.first.count("S") > static_cast<std::size_t>(0));
    REQUIRE_EQ(pair.first.at("S").at("k"), std::string("1"));
    REQUIRE(pair.second.ranAny() == false);
    return true;
}

static bool test_migration_chain_applied_in_order()
{
    RawConfigMap raw;
    raw["S"]["a"] = "0";

    auto m1   = rename(1, 2, "S", "a", "S", "b");
    auto m2   = rename(2, 3, "S", "b", "S", "c");
    auto pair  = applyMigrations(raw, {m1, m2}, 1, 3);
    const RawConfigMap& result = pair.first;
    const auto& info           = pair.second;

    REQUIRE(result.at("S").count("a") == static_cast<std::size_t>(0));
    REQUIRE(result.at("S").count("b") == static_cast<std::size_t>(0));
    REQUIRE_EQ(result.at("S").at("c"), std::string("0"));
    REQUIRE_EQ(static_cast<int>(info.changes.size()), 2);
    return true;
}

static bool test_parseSchemaVersion_reads_from_file()
{
    const std::string path = "configlib_test_parseversion.ini";
    {
        std::ofstream f(path);
        f << "# __schema_version__ = 7\n\n[S]\nkey = val\n";
    }
    const uint32_t v = parseSchemaVersion(path);
    (void)std::remove(path.c_str());
    REQUIRE_EQ(v, static_cast<uint32_t>(7));
    return true;
}

static bool test_parseSchemaVersion_missing_returns_zero()
{
    const std::string path = "configlib_test_noversion.ini";
    {
        std::ofstream f(path);
        f << "[S]\nkey = val\n";
    }
    const uint32_t v = parseSchemaVersion(path);
    (void)std::remove(path.c_str());
    REQUIRE_EQ(v, invalidSchemaVersion);
    return true;
}

static bool test_parseSchemaVersion_nonexistent_file_returns_zero()
{
    const uint32_t v = parseSchemaVersion("configlib_no_such_file_xyz.ini");
    REQUIRE_EQ(v, invalidSchemaVersion);
    return true;
}

static bool test_migrationResult_ranAny()
{
    RawConfigMap raw;
    raw["S"]["k"] = "1";
    auto noOp = applyMigrations(raw, {}, 1, 1);
    REQUIRE(noOp.second.ranAny() == false);

    auto m      = rename(1, 2, "S", "k", "S", "k2");
    auto withOp = applyMigrations(raw, {m}, 1, 2);
    REQUIRE(withOp.second.ranAny() == true);
    return true;
}

static bool test_logMigrationResult_no_migrations_writes_output()
{
    RawConfigMap raw;
    auto pair = applyMigrations(raw, {}, 1, 1);
    std::ostringstream out;
    logMigrationResult(pair.second, "test.ini", out);
    REQUIRE(!out.str().empty());
    return true;
}

static bool test_logMigrationResult_with_changes_writes_output()
{
    RawConfigMap raw;
    raw["S"]["k"] = "5";
    auto m    = rename(1, 2, "S", "k", "S", "k2");
    auto pair = applyMigrations(raw, {m}, 1, 2);
    std::ostringstream out;
    logMigrationResult(pair.second, "test.ini", out);
    REQUIRE(!out.str().empty());
    return true;
}

static bool test_rename_source_key_missing_is_no_op()
{
    RawConfigMap raw;
    raw["S"]["other"] = "1";

    auto m    = rename(1, 2, "S", "missing_key", "S", "new_key");
    auto pair  = applyMigrations(raw, {m}, 1, 2);
    REQUIRE(pair.second.ranAny() == false);
    REQUIRE_EQ(pair.first.at("S").at("other"), std::string("1"));
    REQUIRE(pair.first.at("S").count("new_key") == static_cast<std::size_t>(0));
    return true;
}

static bool test_renameSection_source_section_missing_is_no_op()
{
    RawConfigMap raw;
    raw["Other"]["key"] = "1";

    auto m    = renameSection(1, 2, "Missing", "Dest");
    auto pair  = applyMigrations(raw, {m}, 1, 2);
    REQUIRE(pair.second.ranAny() == false);
    REQUIRE(pair.first.count("Dest") == static_cast<std::size_t>(0));
    REQUIRE_EQ(pair.first.at("Other").at("key"), std::string("1"));
    return true;
}

static bool test_parseSchemaVersion_bad_version_string_returns_zero()
{
    const std::string path = "configlib_test_badversion.ini";
    {
        std::ofstream f(path);
        f << "# __schema_version__ = notanumber\n[S]\nkey = val\n";
    }
    const uint32_t v = parseSchemaVersion(path);
    (void)std::remove(path.c_str());
    REQUIRE_EQ(v, invalidSchemaVersion);
    return true;
}

static bool test_logMigrationResult_rename_transform_mentions_keys()
{
    RawConfigMap raw;
    raw["A"]["k"] = "10";
    auto m = rename(1, 2, "A", "k", "B", "j",
                    [](const std::string& val) { return val + "x"; });
    auto pair = applyMigrations(raw, {m}, 1, 2);
    std::ostringstream out;
    logMigrationResult(pair.second, "cfg.ini", out);
    const std::string s = out.str();
    REQUIRE(s.find("cfg.ini") != std::string::npos);
    REQUIRE(s.find("A.k") != std::string::npos);
    return true;
}

int main()
{
    return runTests({
        {"applyMigrations: none returns unchanged",              test_applyMigrations_none_returns_unchanged},
        {"rename: moves value",                                  test_rename_moves_value},
        {"rename: with transform",                               test_rename_with_transform},
        {"rename: source key missing is no-op",                  test_rename_source_key_missing_is_no_op},
        {"transformInPlace: applies in place",                   test_transformInPlace_applies_in_place},
        {"transformInPlace: null fn throws",                     test_transformInPlace_null_fn_throws},
        {"transformKey: null fn throws",                         test_transformKey_null_fn_throws},
        {"renameSection: moves all keys",                        test_renameSection_moves_all_keys},
        {"renameSection: with renameKey child",                  test_renameSection_with_renameKey_child},
        {"renameSection: with transformKey child",               test_renameSection_with_transformKey_child},
        {"renameSection: source section missing is no-op",       test_renameSection_source_section_missing_is_no_op},
        {"renameKey: with transform",                            test_renameKey_with_transform},
        {"migration out of range not applied",                   test_migration_out_of_version_range_not_applied},
        {"migration chain applied in order",                     test_migration_chain_applied_in_order},
        {"parseSchemaVersion: reads version from file",          test_parseSchemaVersion_reads_from_file},
        {"parseSchemaVersion: missing returns zero",             test_parseSchemaVersion_missing_returns_zero},
        {"parseSchemaVersion: nonexistent file returns zero",    test_parseSchemaVersion_nonexistent_file_returns_zero},
        {"parseSchemaVersion: bad version string returns zero",  test_parseSchemaVersion_bad_version_string_returns_zero},
        {"MigrationResult::ranAny",                              test_migrationResult_ranAny},
        {"logMigrationResult: no migrations",                    test_logMigrationResult_no_migrations_writes_output},
        {"logMigrationResult: with changes",                     test_logMigrationResult_with_changes_writes_output},
        {"logMigrationResult: rename+transform mentions keys",   test_logMigrationResult_rename_transform_mentions_keys},
    });
}
