#include "test_framework.hpp"
#include "config_library/common/type_parser.hpp"
#include "config_library/common/config_value.hpp"

using ConfigLib::TypeParser;
using ConfigLib::TypeRegistry;

static bool test_int_fromString()
{
    REQUIRE_EQ(TypeParser<int>::fromString("42"), 42);
    REQUIRE_EQ(TypeParser<int>::fromString("-7"), -7);
    REQUIRE_EQ(TypeParser<int>::fromString("0"), 0);
    return true;
}

static bool test_int_toString()
{
    REQUIRE_EQ(TypeParser<int>::toString(42), std::string("42"));
    REQUIRE_EQ(TypeParser<int>::toString(-7), std::string("-7"));
    return true;
}

static bool test_int_typeName()
{
    REQUIRE_EQ(std::string(TypeParser<int>::typeName()), std::string("int"));
    return true;
}

static bool test_int_isValid()
{
    REQUIRE(TypeParser<int>::isValid("123") == true);
    REQUIRE(TypeParser<int>::isValid("abc") == false);
    return true;
}

static bool test_double_fromString()
{
    double v = TypeParser<double>::fromString("3.14");
    REQUIRE(v > 3.13 && v < 3.15);
    return true;
}

static bool test_double_toString()
{
    std::string s = TypeParser<double>::toString(1.5);
    REQUIRE(!s.empty());
    return true;
}

static bool test_double_typeName()
{
    REQUIRE_EQ(std::string(TypeParser<double>::typeName()), std::string("double"));
    return true;
}

static bool test_bool_fromString_true_variants()
{
    REQUIRE(TypeParser<bool>::fromString("true")  == true);
    REQUIRE(TypeParser<bool>::fromString("1")     == true);
    REQUIRE(TypeParser<bool>::fromString("yes")   == true);
    return true;
}

static bool test_bool_fromString_false_variants()
{
    REQUIRE(TypeParser<bool>::fromString("false") == false);
    REQUIRE(TypeParser<bool>::fromString("0")     == false);
    REQUIRE(TypeParser<bool>::fromString("no")    == false);
    return true;
}

static bool test_bool_toString()
{
    REQUIRE_EQ(TypeParser<bool>::toString(true),  std::string("true"));
    REQUIRE_EQ(TypeParser<bool>::toString(false), std::string("false"));
    return true;
}

static bool test_bool_typeName()
{
    REQUIRE_EQ(std::string(TypeParser<bool>::typeName()), std::string("bool"));
    return true;
}

static bool test_string_roundtrip()
{
    const std::string s = "hello world";
    REQUIRE_EQ(TypeParser<std::string>::fromString(s), s);
    REQUIRE_EQ(TypeParser<std::string>::toString(s), s);
    return true;
}

static bool test_string_typeName()
{
    REQUIRE_EQ(std::string(TypeParser<std::string>::typeName()), std::string("string"));
    return true;
}

static bool test_vector_int_fromString()
{
    auto v = TypeParser<std::vector<int>>::fromString("1,2,3");
    REQUIRE_EQ(static_cast<int>(v.size()), 3);
    REQUIRE_EQ(v[0], 1);
    REQUIRE_EQ(v[2], 3);
    return true;
}

static bool test_registry_has_builtin_types()
{
    auto& reg = TypeRegistry::instance();
    REQUIRE(reg.hasType("int")    == true);
    REQUIRE(reg.hasType("double") == true);
    REQUIRE(reg.hasType("bool")   == true);
    REQUIRE(reg.hasType("string") == true);
    REQUIRE(reg.hasType("float")  == true);
    REQUIRE(reg.hasType("UnknownType") == false);
    return true;
}

static bool test_registry_parseValue_int()
{
    auto& reg = TypeRegistry::instance();
    auto val  = reg.parseValue("int", "99");
    REQUIRE(val != nullptr);
    REQUIRE_EQ(val->toString(), std::string("99"));
    return true;
}

static bool test_registry_parseValue_bool()
{
    auto& reg = TypeRegistry::instance();
    auto val  = reg.parseValue("bool", "true");
    REQUIRE(val != nullptr);
    REQUIRE_EQ(val->toString(), std::string("true"));
    return true;
}

static bool test_registry_parseValue_throws_unknown_type()
{
    auto& reg = TypeRegistry::instance();
    REQUIRE_THROWS(reg.parseValue("NoSuchType", "42"));
    return true;
}

static bool test_registry_validateValue()
{
    auto& reg = TypeRegistry::instance();
    REQUIRE(reg.validateValue("int",    "42")  == true);
    REQUIRE(reg.validateValue("int",    "abc") == false);
    REQUIRE(reg.validateValue("double", "3.14") == true);
    return true;
}

static bool test_registry_getRegisteredTypesString()
{
    auto& reg = TypeRegistry::instance();
    std::string s = reg.getRegisteredTypesString();
    REQUIRE(!s.empty());
    return true;
}

static bool test_int_fromString_throws_on_bad_input()
{
    REQUIRE_THROWS(TypeParser<int>::fromString("not_a_number"));
    return true;
}

int main()
{
    return runTests({
        {"TypeParser<int>: fromString",              test_int_fromString},
        {"TypeParser<int>: toString",                test_int_toString},
        {"TypeParser<int>: typeName",                test_int_typeName},
        {"TypeParser<int>: isValid",                 test_int_isValid},
        {"TypeParser<int>: fromString throws on bad input", test_int_fromString_throws_on_bad_input},
        {"TypeParser<double>: fromString",           test_double_fromString},
        {"TypeParser<double>: toString",             test_double_toString},
        {"TypeParser<double>: typeName",             test_double_typeName},
        {"TypeParser<bool>: true variants",          test_bool_fromString_true_variants},
        {"TypeParser<bool>: false variants",         test_bool_fromString_false_variants},
        {"TypeParser<bool>: toString",               test_bool_toString},
        {"TypeParser<bool>: typeName",               test_bool_typeName},
        {"TypeParser<string>: roundtrip",            test_string_roundtrip},
        {"TypeParser<string>: typeName",             test_string_typeName},
        {"TypeParser<vector<int>>: fromString",      test_vector_int_fromString},
        {"TypeRegistry: has builtin types",          test_registry_has_builtin_types},
        {"TypeRegistry: parseValue int",             test_registry_parseValue_int},
        {"TypeRegistry: parseValue bool",            test_registry_parseValue_bool},
        {"TypeRegistry: parseValue unknown throws",  test_registry_parseValue_throws_unknown_type},
        {"TypeRegistry: validateValue",              test_registry_validateValue},
        {"TypeRegistry: getRegisteredTypesString",   test_registry_getRegisteredTypesString},
    });
}
