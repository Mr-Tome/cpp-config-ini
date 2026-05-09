#include "test_framework.hpp"
#include "config_library/common/config_schema.hpp"

static bool test_make_int_sets_type_and_default()
{
    auto item = ConfigLib::ConfigItem::make<int>("count", 42, "a count");
    REQUIRE_EQ(item.name,         std::string("count"));
    REQUIRE_EQ(item.type,         std::string("int"));
    REQUIRE_EQ(item.defaultValue, std::string("42"));
    REQUIRE_EQ(item.description,  std::string("a count"));
    REQUIRE(item.validationRule == nullptr);
    REQUIRE_EQ(item.persistence, ConfigLib::Persistence::Normal);
    return true;
}

static bool test_make_double_sets_type_and_default()
{
    auto item = ConfigLib::ConfigItem::make<double>("ratio", 0.5, "a ratio");
    REQUIRE_EQ(item.type, std::string("double"));
    // default value should round-trip through TypeParser
    REQUIRE(!item.defaultValue.empty());
    return true;
}

static bool test_make_bool_sets_type()
{
    auto item = ConfigLib::ConfigItem::make<bool>("flag", false, "a flag");
    REQUIRE_EQ(item.type, std::string("bool"));
    return true;
}

static bool test_make_string_sets_type_and_default()
{
    auto item = ConfigLib::ConfigItem::make<std::string>(
        "name", std::string("hello"), "a name");
    REQUIRE_EQ(item.type,         std::string("string"));
    REQUIRE_EQ(item.defaultValue, std::string("hello"));
    return true;
}

static bool test_make_with_validation_rule()
{
    auto item = ConfigLib::ConfigItem::make<int>(
        "threads", 4, "thread count", &ValidationRules::greaterThanZero);
    REQUIRE(item.validationRule != nullptr);
    return true;
}

static bool test_make_volatile_persistence()
{
    auto item = ConfigLib::ConfigItem::make<std::string>(
        "run_id", std::string(""), "run id", nullptr, ConfigLib::Persistence::Volatile);
    REQUIRE_EQ(item.persistence, ConfigLib::Persistence::Volatile);
    return true;
}

static bool test_config_section_has_name_and_items()
{
    ConfigLib::ConfigSection section;
    section.name = "MySection";
    section.items.push_back(
        ConfigLib::ConfigItem::make<int>("value", 10, "desc"));
    REQUIRE_EQ(section.name, std::string("MySection"));
    REQUIRE_EQ(static_cast<int>(section.items.size()), 1);
    REQUIRE_EQ(section.items[0].name, std::string("value"));
    return true;
}

static bool test_persistence_enum_values_distinct()
{
    REQUIRE(ConfigLib::Persistence::Normal != ConfigLib::Persistence::Volatile);
    return true;
}

int main()
{
    return runTests({
        {"ConfigItem::make<int>",                 test_make_int_sets_type_and_default},
        {"ConfigItem::make<double>",              test_make_double_sets_type_and_default},
        {"ConfigItem::make<bool>",                test_make_bool_sets_type},
        {"ConfigItem::make<string>",              test_make_string_sets_type_and_default},
        {"ConfigItem::make with validation rule", test_make_with_validation_rule},
        {"ConfigItem::make Volatile persistence", test_make_volatile_persistence},
        {"ConfigSection has name and items",      test_config_section_has_name_and_items},
        {"Persistence enum values are distinct",  test_persistence_enum_values_distinct},
    });
}
