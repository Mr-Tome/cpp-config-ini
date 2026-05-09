#include "test_framework.hpp"
#include "config_library/common/schema_evolver.hpp"
#include "config_library/common/config_schema.hpp"

using ConfigLib::ConfigSection;
using ConfigLib::ConfigItem;
using ConfigLib::RawConfigMap;
using ConfigLib::SchemaEvolutionResult;
using ConfigLib::OrphanedConfigItemPolicy;
using ConfigLib::ConfigItemConflictType;
using ConfigLib::evolveFileWithSchema;
using ConfigLib::logEvolutionResult;

static std::vector<ConfigSection> makeSchema()
{
    return {{
        "Settings",
        {
            ConfigItem::make<int>("count",   5,     "item count"),
            ConfigItem::make<double>("ratio", 1.0,  "a ratio",      &ValidationRules::greaterThanZero),
            ConfigItem::make<bool>("verbose", false, "verbose mode"),
        }
    }};
}

static bool test_empty_rawConfig_marks_all_as_added()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    auto result = evolveFileWithSchema(raw, schema);

    REQUIRE(result.fileModified == true);
    REQUIRE(static_cast<int>(result.addedSections.size()) == 1);
    REQUIRE_EQ(result.addedSections[0].name, std::string("Settings"));
    REQUIRE_EQ(static_cast<int>(result.addedSections[0].items.size()), 3);
    return true;
}

static bool test_matching_rawConfig_is_clean()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "5";
    raw["Settings"]["ratio"]   = "1.0";
    raw["Settings"]["verbose"] = "false";
    auto result = evolveFileWithSchema(raw, schema);

    REQUIRE(result.fileModified == false);
    REQUIRE(result.isClean() == true);
    REQUIRE(static_cast<int>(result.addedSections.size()) == 0);
    return true;
}

static bool test_type_mismatch_sets_conflict()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "not_a_number";
    raw["Settings"]["ratio"]   = "1.0";
    raw["Settings"]["verbose"] = "false";
    auto result = evolveFileWithSchema(raw, schema);

    REQUIRE(result.fileModified == true);
    REQUIRE(result.numberOfConflicts() > static_cast<std::size_t>(0));

    bool foundMismatch = false;
    for (const auto& sc : result.conflictingSections)
        for (const auto& ci : sc.conflictingConfigItems)
            if (ci.conflictType == ConfigItemConflictType::TypeMismatch)
                foundMismatch = true;
    REQUIRE(foundMismatch == true);
    return true;
}

static bool test_validation_failure_sets_conflict()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "5";
    raw["Settings"]["ratio"]   = "-1.0"; // fails greaterThanZero
    raw["Settings"]["verbose"] = "false";
    auto result = evolveFileWithSchema(raw, schema);

    REQUIRE(result.fileModified == true);
    bool foundFailure = false;
    for (const auto& sc : result.conflictingSections)
        for (const auto& ci : sc.conflictingConfigItems)
            if (ci.conflictType == ConfigItemConflictType::ValidationFailure)
                foundFailure = true;
    REQUIRE(foundFailure == true);
    return true;
}

static bool test_orphaned_key_is_recorded()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]       = "5";
    raw["Settings"]["ratio"]       = "1.0";
    raw["Settings"]["verbose"]     = "false";
    raw["Settings"]["extra_key"]   = "orphan";
    auto result = evolveFileWithSchema(raw, schema, OrphanedConfigItemPolicy::CommentOut);

    REQUIRE(result.fileModified == true);
    bool foundOrphan = false;
    for (const auto& sc : result.conflictingSections)
        for (const auto& orphan : sc.removedEntries)
            if (orphan.configItemName == "extra_key")
                foundOrphan = true;
    REQUIRE(foundOrphan == true);
    return true;
}

static bool test_orphaned_key_with_RuntimeError_throws()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "5";
    raw["Settings"]["ratio"]   = "1.0";
    raw["Settings"]["verbose"] = "false";
    raw["Settings"]["ghost"]   = "42";
    REQUIRE_THROWS(evolveFileWithSchema(raw, schema, OrphanedConfigItemPolicy::RuntimeError));
    return true;
}

static bool test_volatile_key_in_file_marks_modified()
{
    using Item = ConfigItem;
    using P    = ConfigLib::Persistence;
    std::vector<ConfigSection> schema = {{
        "S",
        {
            Item::make<int>("stored",   1, "stored"),
            Item::make<std::string>("vol", std::string(""), "volatile", nullptr, P::Volatile),
        }
    }};
    RawConfigMap raw;
    raw["S"]["stored"] = "1";
    raw["S"]["vol"]    = "should_not_be_here";
    auto result = evolveFileWithSchema(raw, schema);
    REQUIRE(result.fileModified == true);
    return true;
}

static bool test_numberOfConflicts_counts_correctly()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "5";
    raw["Settings"]["ratio"]   = "1.0";
    raw["Settings"]["verbose"] = "false";
    auto clean = evolveFileWithSchema(raw, schema);
    REQUIRE_EQ(clean.numberOfConflicts(), static_cast<std::size_t>(0));

    RawConfigMap raw2; // all items missing → all added
    auto dirty = evolveFileWithSchema(raw2, schema);
    REQUIRE(dirty.numberOfConflicts() > static_cast<std::size_t>(0));
    return true;
}

static bool test_logEvolutionResult_writes_to_stream()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    auto result = evolveFileWithSchema(raw, schema);

    std::ostringstream out;
    logEvolutionResult(result, "dummy.ini", out);
    REQUIRE(!out.str().empty());
    return true;
}

static bool test_orphaned_entire_section_is_recorded()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "5";
    raw["Settings"]["ratio"]   = "1.0";
    raw["Settings"]["verbose"] = "false";
    raw["OldSection"]["legacy"] = "42";
    auto result = evolveFileWithSchema(raw, schema, OrphanedConfigItemPolicy::CommentOut);

    REQUIRE(result.fileModified == true);
    bool foundOrphan = false;
    for (const auto& sc : result.conflictingSections)
        for (const auto& orphan : sc.removedEntries)
            if (orphan.sectionName == "OldSection" && orphan.configItemName == "legacy")
                foundOrphan = true;
    REQUIRE(foundOrphan == true);
    return true;
}

static bool test_logEvolutionResult_content_mentions_filename_and_added_key()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    auto result = evolveFileWithSchema(raw, schema);

    std::ostringstream out;
    logEvolutionResult(result, "myconfig.ini", out);
    const std::string s = out.str();
    REQUIRE(s.find("myconfig.ini") != std::string::npos);
    REQUIRE(s.find("count") != std::string::npos);
    return true;
}

static bool test_logEvolutionResult_type_mismatch_result_is_nonempty()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "not_a_number";
    raw["Settings"]["ratio"]   = "1.0";
    raw["Settings"]["verbose"] = "false";
    auto result = evolveFileWithSchema(raw, schema);

    REQUIRE(result.numberOfConflicts() > static_cast<std::size_t>(0));

    std::ostringstream out;
    logEvolutionResult(result, "cfg.ini", out);
    REQUIRE(!out.str().empty());
    return true;
}

static bool test_logEvolutionResult_orphaned_key_result_is_nonempty()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]     = "5";
    raw["Settings"]["ratio"]     = "1.0";
    raw["Settings"]["verbose"]   = "false";
    raw["Settings"]["extra_key"] = "orphan";
    auto result = evolveFileWithSchema(raw, schema, OrphanedConfigItemPolicy::CommentOut);

    REQUIRE(result.fileModified == true);

    std::ostringstream out;
    logEvolutionResult(result, "cfg.ini", out);
    REQUIRE(!out.str().empty());
    return true;
}

static bool test_isClean_returns_false_after_type_mismatch()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "not_a_number";
    raw["Settings"]["ratio"]   = "1.0";
    raw["Settings"]["verbose"] = "false";
    auto result = evolveFileWithSchema(raw, schema);
    REQUIRE(result.isClean() == false);
    return true;
}

static bool test_isClean_returns_false_after_validation_failure()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "5";
    raw["Settings"]["ratio"]   = "-1.0"; // fails greaterThanZero
    raw["Settings"]["verbose"] = "false";
    auto result = evolveFileWithSchema(raw, schema);
    REQUIRE(result.isClean() == false);
    return true;
}

static bool test_orphaned_item_fields_are_correct()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]      = "5";
    raw["Settings"]["ratio"]      = "1.0";
    raw["Settings"]["verbose"]    = "false";
    raw["Settings"]["legacy_key"] = "42";
    auto result = evolveFileWithSchema(raw, schema, OrphanedConfigItemPolicy::CommentOut);

    bool found = false;
    for (const auto& sc : result.conflictingSections)
        for (const auto& orphan : sc.removedEntries)
            if (orphan.configItemName == "legacy_key")
            {
                REQUIRE_EQ(orphan.sectionName, std::string("Settings"));
                REQUIRE_EQ(orphan.rawValue,    std::string("42"));
                found = true;
            }
    REQUIRE(found == true);
    return true;
}

static bool test_evolve_orphaned_policy_remove()
{
    auto schema = makeSchema();
    RawConfigMap raw;
    raw["Settings"]["count"]   = "5";
    raw["Settings"]["ratio"]   = "1.0";
    raw["Settings"]["verbose"] = "false";
    raw["Settings"]["ghost"]   = "stale_value";
    auto result = evolveFileWithSchema(raw, schema, OrphanedConfigItemPolicy::Remove);

    REQUIRE(result.fileModified == true);
    bool found = false;
    for (const auto& sc : result.conflictingSections)
        for (const auto& orphan : sc.removedEntries)
            if (orphan.configItemName == "ghost")
                found = true;
    REQUIRE(found == true);
    return true;
}

static bool test_orphaned_section_RuntimeError_throws()
{
    auto schema = makeSchema(); // only "Settings" section
    RawConfigMap raw;
    raw["Settings"]["count"]    = "5";
    raw["Settings"]["ratio"]    = "1.0";
    raw["Settings"]["verbose"]  = "false";
    raw["OldSection"]["legacy"] = "42"; // entire section not in schema
    REQUIRE_THROWS(evolveFileWithSchema(raw, schema, OrphanedConfigItemPolicy::RuntimeError));
    return true;
}

int main()
{
    return runTests({
        {"empty rawConfig: all items added",                      test_empty_rawConfig_marks_all_as_added},
        {"matching rawConfig: isClean",                           test_matching_rawConfig_is_clean},
        {"type mismatch sets TypeMismatch conflict",              test_type_mismatch_sets_conflict},
        {"validation failure sets conflict",                      test_validation_failure_sets_conflict},
        {"orphaned key is recorded",                              test_orphaned_key_is_recorded},
        {"orphaned key + RuntimeError throws",                    test_orphaned_key_with_RuntimeError_throws},
        {"orphaned entire section is recorded",                   test_orphaned_entire_section_is_recorded},
        {"volatile key in file marks fileModified",               test_volatile_key_in_file_marks_modified},
        {"numberOfConflicts counts correctly",                    test_numberOfConflicts_counts_correctly},
        {"logEvolutionResult writes to stream",                   test_logEvolutionResult_writes_to_stream},
        {"logEvolutionResult mentions filename and added key",    test_logEvolutionResult_content_mentions_filename_and_added_key},
        {"logEvolutionResult: type mismatch output is non-empty",  test_logEvolutionResult_type_mismatch_result_is_nonempty},
        {"logEvolutionResult: orphaned key output is non-empty",   test_logEvolutionResult_orphaned_key_result_is_nonempty},
        {"isClean: false after type mismatch",                     test_isClean_returns_false_after_type_mismatch},
        {"isClean: false after validation failure",                 test_isClean_returns_false_after_validation_failure},
        {"orphaned item: fields sectionName/configItemName/rawValue", test_orphaned_item_fields_are_correct},
        {"OrphanedConfigItemPolicy::Remove records orphan",        test_evolve_orphaned_policy_remove},
        {"OrphanedPolicy::RuntimeError on orphaned section throws", test_orphaned_section_RuntimeError_throws},
    });
}
