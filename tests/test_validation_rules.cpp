#include "test_framework.hpp"
#include "config_library/common/validation_rules.hpp"
#include "config_library/common/config_value.hpp"

// Helper: create a typed numeric config value usable by Rule::operator()
static ConfigLib::TypedConfigValue<int>    intVal(int v)    { return ConfigLib::TypedConfigValue<int>(v); }
static ConfigLib::TypedConfigValue<double> dblVal(double v) { return ConfigLib::TypedConfigValue<double>(v); }
static ConfigLib::TypedConfigValue<std::string> strVal(const std::string& v)
{
    return ConfigLib::TypedConfigValue<std::string>(v);
}

static bool test_greaterThanZero_passes_positive()
{
    auto v = intVal(1);
    REQUIRE(ValidationRules::greaterThanZero(v) == true);
    auto d = dblVal(0.001);
    REQUIRE(ValidationRules::greaterThanZero(d) == true);
    return true;
}

static bool test_greaterThanZero_fails_zero()
{
    auto v = intVal(0);
    REQUIRE(ValidationRules::greaterThanZero(v) == false);
    return true;
}

static bool test_greaterThanZero_fails_negative()
{
    auto v = intVal(-5);
    REQUIRE(ValidationRules::greaterThanZero(v) == false);
    return true;
}

static bool test_greaterThanZero_toString()
{
    REQUIRE(!ValidationRules::greaterThanZero.toString().empty());
    return true;
}

static bool test_greaterThanOrEqualToZero_passes_zero()
{
    auto v = intVal(0);
    REQUIRE(ValidationRules::greaterThanOrEqualToZero(v) == true);
    return true;
}

static bool test_greaterThanOrEqualToZero_passes_positive()
{
    auto v = intVal(10);
    REQUIRE(ValidationRules::greaterThanOrEqualToZero(v) == true);
    return true;
}

static bool test_greaterThanOrEqualToZero_fails_negative()
{
    auto v = intVal(-1);
    REQUIRE(ValidationRules::greaterThanOrEqualToZero(v) == false);
    return true;
}

static bool test_greaterThanOrEqualToZero_toString()
{
    REQUIRE(!ValidationRules::greaterThanOrEqualToZero.toString().empty());
    return true;
}

static bool test_betweenValues_passes_in_range()
{
    auto rule = ValidationRules::betweenValues(0.0, 10.0);
    auto v    = dblVal(5.0);
    REQUIRE((*rule)(v) == true);
    auto lo = dblVal(0.0);
    REQUIRE((*rule)(lo) == true);
    auto hi = dblVal(10.0);
    REQUIRE((*rule)(hi) == true);
    return true;
}

static bool test_betweenValues_fails_below()
{
    auto rule = ValidationRules::betweenValues(1.0, 10.0);
    auto v    = dblVal(0.5);
    REQUIRE((*rule)(v) == false);
    return true;
}

static bool test_betweenValues_fails_above()
{
    auto rule = ValidationRules::betweenValues(0.0, 5.0);
    auto v    = dblVal(5.1);
    REQUIRE((*rule)(v) == false);
    return true;
}

static bool test_betweenValues_toString()
{
    auto rule = ValidationRules::betweenValues(0.0, 100.0);
    REQUIRE(!rule->toString().empty());
    return true;
}

static bool test_inList_passes_valid()
{
    auto rule = ValidationRules::inList({"red", "green", "blue"});
    auto v    = strVal("green");
    REQUIRE((*rule)(v) == true);
    return true;
}

static bool test_inList_fails_invalid()
{
    auto rule = ValidationRules::inList({"red", "green", "blue"});
    auto v    = strVal("yellow");
    REQUIRE((*rule)(v) == false);
    return true;
}

static bool test_inList_toString()
{
    auto rule = ValidationRules::inList({"a", "b"});
    REQUIRE(!rule->toString().empty());
    return true;
}

static bool test_greaterThanZero_with_string_returns_false()
{
    auto v = strVal("hello");
    REQUIRE(ValidationRules::greaterThanZero(v) == false);
    return true;
}

static bool test_betweenValues_with_string_returns_false()
{
    auto rule = ValidationRules::betweenValues(0.0, 10.0);
    auto v    = strVal("five");
    REQUIRE((*rule)(v) == false);
    return true;
}

static bool test_inList_with_int_value_returns_false()
{
    auto rule = ValidationRules::inList({"red", "green"});
    auto v    = intVal(1);
    REQUIRE((*rule)(v) == false);
    return true;
}

static bool test_inList_empty_list_always_fails()
{
    auto rule = ValidationRules::inList({});
    auto v    = strVal("anything");
    REQUIRE((*rule)(v) == false);
    return true;
}

static bool test_betweenValues_min_equals_max()
{
    auto rule  = ValidationRules::betweenValues(5.0, 5.0);
    auto exact = dblVal(5.0);
    auto below = dblVal(4.999);
    auto above = dblVal(5.001);
    REQUIRE((*rule)(exact) == true);
    REQUIRE((*rule)(below) == false);
    REQUIRE((*rule)(above) == false);
    return true;
}

static bool test_inList_single_element_passes_and_fails()
{
    auto rule = ValidationRules::inList({"only"});
    REQUIRE(!rule->toString().empty());
    auto match  = strVal("only");
    auto nomatch = strVal("other");
    REQUIRE((*rule)(match)   == true);
    REQUIRE((*rule)(nomatch) == false);
    return true;
}

int main()
{
    return runTests({
        {"greaterThanZero: positive passes",          test_greaterThanZero_passes_positive},
        {"greaterThanZero: zero fails",               test_greaterThanZero_fails_zero},
        {"greaterThanZero: negative fails",           test_greaterThanZero_fails_negative},
        {"greaterThanZero: toString non-empty",       test_greaterThanZero_toString},
        {"greaterThanOrEqualToZero: zero passes",     test_greaterThanOrEqualToZero_passes_zero},
        {"greaterThanOrEqualToZero: positive passes", test_greaterThanOrEqualToZero_passes_positive},
        {"greaterThanOrEqualToZero: negative fails",  test_greaterThanOrEqualToZero_fails_negative},
        {"greaterThanOrEqualToZero: toString",         test_greaterThanOrEqualToZero_toString},
        {"betweenValues: in range passes",            test_betweenValues_passes_in_range},
        {"betweenValues: below range fails",          test_betweenValues_fails_below},
        {"betweenValues: above range fails",          test_betweenValues_fails_above},
        {"betweenValues: toString non-empty",         test_betweenValues_toString},
        {"inList: valid value passes",                test_inList_passes_valid},
        {"inList: invalid value fails",               test_inList_fails_invalid},
        {"inList: toString non-empty",                test_inList_toString},
        {"greaterThanZero: string value returns false",  test_greaterThanZero_with_string_returns_false},
        {"betweenValues: string value returns false",    test_betweenValues_with_string_returns_false},
        {"inList: int value returns false",              test_inList_with_int_value_returns_false},
        {"inList: empty list always fails",              test_inList_empty_list_always_fails},
        {"betweenValues: min == max boundary",           test_betweenValues_min_equals_max},
        {"inList: single element passes and fails",      test_inList_single_element_passes_and_fails},
    });
}
