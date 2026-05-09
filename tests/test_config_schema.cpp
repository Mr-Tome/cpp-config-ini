#include "test_framework.hpp"
#include "config_library/common/config_schema.hpp"

static std::vector<ConfigLib::ConfigSection> makeSimpleSchema()
{
    using Item = ConfigLib::ConfigItem;
    return {{
        "Alpha",
        {
            Item::make<int>("x", 1, "x value"),
            Item::make<double>("y", 2.0, "y value"),
        }
    }};
}

static bool test_validateConfig_valid_schema()
{
    REQUIRE(ConfigLib::validateConfig(makeSimpleSchema()) == true);
    return true;
}

static bool test_validateConfig_unknown_type_returns_false()
{
    ConfigLib::ConfigSection bad;
    bad.name = "S";
    bad.items.push_back(ConfigLib::ConfigItem("key", "UnknownType", "0", "desc", nullptr));
    REQUIRE(ConfigLib::validateConfig({bad}) == false);
    return true;
}

static bool test_mergeDuplicateSections_combines_items()
{
    using Item = ConfigLib::ConfigItem;
    std::vector<ConfigLib::ConfigSection> sections = {
        {"S", {Item::make<int>("a", 1, "a")}},
        {"S", {Item::make<int>("b", 2, "b")}},
    };
    auto merged = ConfigLib::mergeDuplicateSections(sections);
    REQUIRE_EQ(static_cast<int>(merged.size()), 1);
    REQUIRE_EQ(merged[0].name, std::string("S"));
    REQUIRE_EQ(static_cast<int>(merged[0].items.size()), 2);
    return true;
}

static bool test_mergeDuplicateSections_different_sections_kept_separate()
{
    using Item = ConfigLib::ConfigItem;
    std::vector<ConfigLib::ConfigSection> sections = {
        {"A", {Item::make<int>("x", 0, "x")}},
        {"B", {Item::make<int>("y", 0, "y")}},
    };
    auto merged = ConfigLib::mergeDuplicateSections(sections);
    REQUIRE_EQ(static_cast<int>(merged.size()), 2);
    return true;
}

static bool test_mergeDuplicateSections_duplicate_key_throws()
{
    using Item = ConfigLib::ConfigItem;
    std::vector<ConfigLib::ConfigSection> sections = {
        {"S", {Item::make<int>("dup", 1, "first")}},
        {"S", {Item::make<int>("dup", 2, "second")}},
    };
    REQUIRE_THROWS(ConfigLib::mergeDuplicateSections(sections));
    return true;
}

static bool test_mergeDuplicateSections_empty_sections_name()
{
    using Item = ConfigLib::ConfigItem;
    std::vector<ConfigLib::ConfigSection> sections = {
        {"", {Item::make<int>("a", 0, "a")}},
        {"", {Item::make<int>("b", 0, "b")}},
    };
    auto merged = ConfigLib::mergeDuplicateSections(sections);
    REQUIRE_EQ(static_cast<int>(merged.size()), 1);
    REQUIRE_EQ(static_cast<int>(merged[0].items.size()), 2);
    return true;
}

static bool test_validateConfig_empty_schema_is_valid()
{
    REQUIRE(ConfigLib::validateConfig({}) == true);
    return true;
}

static bool test_buildSchemaLookup_contains_sections_and_keys()
{
    using Item = ConfigLib::ConfigItem;
    std::vector<ConfigLib::ConfigSection> sections = {
        {"Alpha", {Item::make<int>("x", 0, "x"), Item::make<int>("y", 0, "y")}},
        {"Beta",  {Item::make<int>("z", 0, "z")}},
    };
    auto lookup = ConfigLib::Internal::buildSchemaLookup(sections);
    REQUIRE(lookup.count("Alpha") > static_cast<std::size_t>(0));
    REQUIRE(lookup.at("Alpha").count("x") > static_cast<std::size_t>(0));
    REQUIRE(lookup.at("Alpha").count("y") > static_cast<std::size_t>(0));
    REQUIRE(lookup.count("Beta") > static_cast<std::size_t>(0));
    REQUIRE(lookup.at("Beta").count("z") > static_cast<std::size_t>(0));
    REQUIRE(lookup.at("Beta").count("w") == static_cast<std::size_t>(0));
    return true;
}

static bool test_buildSchemaItemLookup_returns_correct_pointers()
{
    using Item = ConfigLib::ConfigItem;
    std::vector<ConfigLib::ConfigSection> sections = {
        {"S", {Item::make<int>("count", 5, "item count")}},
    };
    auto lookup = ConfigLib::Internal::buildSchemaItemLookup(sections);
    REQUIRE(lookup.count("S") > static_cast<std::size_t>(0));
    REQUIRE(lookup.at("S").count("count") > static_cast<std::size_t>(0));
    const ConfigLib::ConfigItem* ptr = lookup.at("S").at("count");
    REQUIRE(ptr != nullptr);
    REQUIRE_EQ(ptr->name, std::string("count"));
    REQUIRE_EQ(ptr->defaultValue, std::string("5"));
    return true;
}

static bool test_validateConfig_invalid_default_value_returns_false()
{
    ConfigLib::ConfigSection bad;
    bad.name = "S";
    bad.items.push_back(ConfigLib::ConfigItem("key", "int", "notanumber", "desc", nullptr));
    REQUIRE(ConfigLib::validateConfig({bad}) == false);
    return true;
}

static bool test_mergeDuplicateSections_three_same_name()
{
    using Item = ConfigLib::ConfigItem;
    std::vector<ConfigLib::ConfigSection> sections = {
        {"S", {Item::make<int>("a", 1, "a")}},
        {"S", {Item::make<int>("b", 2, "b")}},
        {"S", {Item::make<int>("c", 3, "c")}},
    };
    auto merged = ConfigLib::mergeDuplicateSections(sections);
    REQUIRE_EQ(static_cast<int>(merged.size()), 1);
    REQUIRE_EQ(merged[0].name, std::string("S"));
    REQUIRE_EQ(static_cast<int>(merged[0].items.size()), 3);
    return true;
}

static bool test_validateConfig_multiple_errors_returns_false()
{
    using Item = ConfigLib::ConfigItem;
    std::vector<ConfigLib::ConfigSection> sections = {{
        "S",
        {
            Item("key1", "NoSuchType",  "0",           "desc", nullptr),
            Item("key2", "int",         "notanumber",  "desc", nullptr),
        }
    }};
    REQUIRE(ConfigLib::validateConfig(sections) == false);
    return true;
}

int main()
{
    return runTests({
        {"validateConfig: valid schema returns true",        test_validateConfig_valid_schema},
        {"validateConfig: unknown type returns false",       test_validateConfig_unknown_type_returns_false},
        {"validateConfig: empty schema is valid",            test_validateConfig_empty_schema_is_valid},
        {"mergeDuplicateSections: combines items",           test_mergeDuplicateSections_combines_items},
        {"mergeDuplicateSections: distinct sections kept",   test_mergeDuplicateSections_different_sections_kept_separate},
        {"mergeDuplicateSections: duplicate key throws",     test_mergeDuplicateSections_duplicate_key_throws},
        {"mergeDuplicateSections: empty section name works", test_mergeDuplicateSections_empty_sections_name},
        {"buildSchemaLookup: sections and keys present",     test_buildSchemaLookup_contains_sections_and_keys},
        {"buildSchemaItemLookup: correct pointers",          test_buildSchemaItemLookup_returns_correct_pointers},
        {"validateConfig: invalid default value returns false", test_validateConfig_invalid_default_value_returns_false},
        {"mergeDuplicateSections: three same-name sections",    test_mergeDuplicateSections_three_same_name},
        {"validateConfig: multiple error types returns false",   test_validateConfig_multiple_errors_returns_false},
    });
}
