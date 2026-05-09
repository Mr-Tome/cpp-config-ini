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

static bool test_int_fromString_throws_on_extra_chars()
{
    REQUIRE_THROWS(TypeParser<int>::fromString("42abc"));
    return true;
}

static bool test_float_roundtrip()
{
    float v = TypeParser<float>::fromString("3.14");
    REQUIRE(v > 3.13f && v < 3.15f);
    REQUIRE(!TypeParser<float>::toString(1.5f).empty());
    REQUIRE_EQ(std::string(TypeParser<float>::typeName()), std::string("float"));
    REQUIRE(TypeParser<float>::isValid("1.5") == true);
    REQUIRE(TypeParser<float>::isValid("bad") == false);
    return true;
}

static bool test_long_roundtrip()
{
    REQUIRE_EQ(TypeParser<long>::fromString("123456"), 123456L);
    REQUIRE_EQ(TypeParser<long>::toString(123456L), std::string("123456"));
    REQUIRE_EQ(std::string(TypeParser<long>::typeName()), std::string("long"));
    return true;
}

static bool test_long_long_roundtrip()
{
    REQUIRE_EQ(TypeParser<long long>::fromString("9876543210"), 9876543210LL);
    REQUIRE(!TypeParser<long long>::toString(9876543210LL).empty());
    REQUIRE_EQ(std::string(TypeParser<long long>::typeName()), std::string("long long"));
    return true;
}

static bool test_unsigned_int_roundtrip()
{
    REQUIRE_EQ(TypeParser<unsigned int>::fromString("42"), 42u);
    REQUIRE_EQ(TypeParser<unsigned int>::toString(42u), std::string("42"));
    REQUIRE_EQ(std::string(TypeParser<unsigned int>::typeName()), std::string("unsigned int"));
    return true;
}

static bool test_unsigned_long_roundtrip()
{
    REQUIRE_EQ(TypeParser<unsigned long>::fromString("999"), 999UL);
    REQUIRE(!TypeParser<unsigned long>::toString(999UL).empty());
    REQUIRE_EQ(std::string(TypeParser<unsigned long>::typeName()), std::string("unsigned long"));
    return true;
}

static bool test_unsigned_long_long_roundtrip()
{
    REQUIRE_EQ(TypeParser<unsigned long long>::fromString("1000"), 1000ULL);
    REQUIRE(!TypeParser<unsigned long long>::toString(1000ULL).empty());
    REQUIRE_EQ(std::string(TypeParser<unsigned long long>::typeName()), std::string("unsigned long long"));
    return true;
}

static bool test_long_double_roundtrip()
{
    long double v = TypeParser<long double>::fromString("2.71");
    REQUIRE(v > 2.70L && v < 2.72L);
    REQUIRE(!TypeParser<long double>::toString(v).empty());
    REQUIRE_EQ(std::string(TypeParser<long double>::typeName()), std::string("long double"));
    return true;
}

static bool test_bool_on_off_variants()
{
    REQUIRE(TypeParser<bool>::fromString("on")  == true);
    REQUIRE(TypeParser<bool>::fromString("off") == false);
    return true;
}

static bool test_bool_fromString_throws_on_invalid()
{
    REQUIRE_THROWS(TypeParser<bool>::fromString("maybe"));
    return true;
}

static bool test_vector_int_toString()
{
    std::vector<int> v = {1, 2, 3};
    REQUIRE_EQ(TypeParser<std::vector<int>>::toString(v), std::string("1,2,3"));
    return true;
}

static bool test_vector_int_invalid_element_throws()
{
    REQUIRE_THROWS(TypeParser<std::vector<int>>::fromString("1,two,3"));
    return true;
}

static bool test_vector_double_roundtrip()
{
    auto v = TypeParser<std::vector<double>>::fromString("1.1,2.2,3.3");
    REQUIRE_EQ(static_cast<int>(v.size()), 3);
    REQUIRE(v[0] > 1.0 && v[0] < 1.2);
    REQUIRE(v[2] > 3.2 && v[2] < 3.4);

    std::vector<double> src = {1.0, 2.0};
    std::string s = TypeParser<std::vector<double>>::toString(src);
    REQUIRE(!s.empty());
    REQUIRE(s.find(',') != std::string::npos);
    REQUIRE_EQ(std::string(TypeParser<std::vector<double>>::typeName()), std::string("vector<double>"));
    return true;
}

static bool test_vector_string_roundtrip()
{
    auto v = TypeParser<std::vector<std::string>>::fromString("alpha,beta,gamma");
    REQUIRE_EQ(static_cast<int>(v.size()), 3);
    REQUIRE_EQ(v[0], std::string("alpha"));
    REQUIRE_EQ(v[2], std::string("gamma"));

    std::vector<std::string> src = {"x", "y"};
    REQUIRE_EQ(TypeParser<std::vector<std::string>>::toString(src), std::string("x,y"));
    REQUIRE_EQ(std::string(TypeParser<std::vector<std::string>>::typeName()), std::string("vector<string>"));
    return true;
}

static bool test_registry_validateValue_unknown_type_returns_false()
{
    auto& reg = TypeRegistry::instance();
    REQUIRE(reg.validateValue("NoSuchType", "42") == false);
    return true;
}

static bool test_registry_getRegisteredTypes_contains_builtins()
{
    const auto& types = TypeRegistry::instance().getRegisteredTypes();
    REQUIRE(types.count("int")    > static_cast<std::size_t>(0));
    REQUIRE(types.count("double") > static_cast<std::size_t>(0));
    REQUIRE(types.count("bool")   > static_cast<std::size_t>(0));
    REQUIRE(types.count("string") > static_cast<std::size_t>(0));
    REQUIRE(types.count("float")  > static_cast<std::size_t>(0));
    return true;
}

static bool test_double_fromString_throws_on_extra_chars()
{
    REQUIRE_THROWS(TypeParser<double>::fromString("3.14abc"));
    return true;
}

static bool test_double_fromString_throws_on_bad_input()
{
    REQUIRE_THROWS(TypeParser<double>::fromString("not_a_number"));
    return true;
}

static bool test_int_fromString_throws_on_out_of_range()
{
    REQUIRE_THROWS(TypeParser<int>::fromString("9999999999")); // exceeds INT_MAX
    return true;
}

static bool test_bool_case_insensitive_true_variants()
{
    REQUIRE(TypeParser<bool>::fromString("TRUE") == true);
    REQUIRE(TypeParser<bool>::fromString("Yes")  == true);
    REQUIRE(TypeParser<bool>::fromString("ON")   == true);
    return true;
}

static bool test_bool_case_insensitive_false_variants()
{
    REQUIRE(TypeParser<bool>::fromString("FALSE") == false);
    REQUIRE(TypeParser<bool>::fromString("No")    == false);
    REQUIRE(TypeParser<bool>::fromString("OFF")   == false);
    return true;
}

static bool test_vector_double_invalid_element_throws()
{
    REQUIRE_THROWS(TypeParser<std::vector<double>>::fromString("1.0,bad,3.0"));
    return true;
}

static bool test_vector_string_elements_are_trimmed()
{
    auto v = TypeParser<std::vector<std::string>>::fromString(" a , b , c ");
    REQUIRE_EQ(static_cast<int>(v.size()), 3);
    REQUIRE_EQ(v[0], std::string("a"));
    REQUIRE_EQ(v[1], std::string("b"));
    REQUIRE_EQ(v[2], std::string("c"));
    return true;
}

static bool test_vector_int_empty_string_gives_empty_vector()
{
    auto v = TypeParser<std::vector<int>>::fromString("");
    REQUIRE_EQ(static_cast<int>(v.size()), 0);
    return true;
}

static bool test_double_fromString_scientific_notation()
{
    double v = TypeParser<double>::fromString("1e5");
    REQUIRE(v > 99999.0 && v < 100001.0);
    double v2 = TypeParser<double>::fromString("1.5e-3");
    REQUIRE(v2 > 0.00149 && v2 < 0.00151);
    return true;
}

static bool test_float_fromString_scientific_notation()
{
    float v = TypeParser<float>::fromString("2.5e2");
    REQUIRE(v > 249.9f && v < 250.1f);
    return true;
}

static bool test_unsigned_long_long_max_value()
{
    REQUIRE_EQ(TypeParser<unsigned long long>::fromString("18446744073709551615"),
               18446744073709551615ULL);
    return true;
}

static bool test_int_isValid_empty_string_returns_false()
{
    REQUIRE(TypeParser<int>::isValid("") == false);
    return true;
}

int main()
{
    return runTests({
        {"TypeParser<int>: fromString",                       test_int_fromString},
        {"TypeParser<int>: toString",                         test_int_toString},
        {"TypeParser<int>: typeName",                         test_int_typeName},
        {"TypeParser<int>: isValid",                          test_int_isValid},
        {"TypeParser<int>: fromString throws on bad input",   test_int_fromString_throws_on_bad_input},
        {"TypeParser<int>: fromString throws on extra chars", test_int_fromString_throws_on_extra_chars},
        {"TypeParser<float>: roundtrip",                      test_float_roundtrip},
        {"TypeParser<long>: roundtrip",                       test_long_roundtrip},
        {"TypeParser<long long>: roundtrip",                  test_long_long_roundtrip},
        {"TypeParser<unsigned int>: roundtrip",               test_unsigned_int_roundtrip},
        {"TypeParser<unsigned long>: roundtrip",              test_unsigned_long_roundtrip},
        {"TypeParser<unsigned long long>: roundtrip",         test_unsigned_long_long_roundtrip},
        {"TypeParser<long double>: roundtrip",                test_long_double_roundtrip},
        {"TypeParser<double>: fromString",                    test_double_fromString},
        {"TypeParser<double>: toString",                      test_double_toString},
        {"TypeParser<double>: typeName",                      test_double_typeName},
        {"TypeParser<bool>: true variants",                   test_bool_fromString_true_variants},
        {"TypeParser<bool>: false variants",                  test_bool_fromString_false_variants},
        {"TypeParser<bool>: on/off variants",                 test_bool_on_off_variants},
        {"TypeParser<bool>: invalid throws",                  test_bool_fromString_throws_on_invalid},
        {"TypeParser<bool>: toString",                        test_bool_toString},
        {"TypeParser<bool>: typeName",                        test_bool_typeName},
        {"TypeParser<string>: roundtrip",                     test_string_roundtrip},
        {"TypeParser<string>: typeName",                      test_string_typeName},
        {"TypeParser<vector<int>>: fromString",               test_vector_int_fromString},
        {"TypeParser<vector<int>>: toString",                 test_vector_int_toString},
        {"TypeParser<vector<int>>: invalid element throws",   test_vector_int_invalid_element_throws},
        {"TypeParser<vector<double>>: roundtrip",             test_vector_double_roundtrip},
        {"TypeParser<vector<string>>: roundtrip",             test_vector_string_roundtrip},
        {"TypeRegistry: has builtin types",                   test_registry_has_builtin_types},
        {"TypeRegistry: parseValue int",                      test_registry_parseValue_int},
        {"TypeRegistry: parseValue bool",                     test_registry_parseValue_bool},
        {"TypeRegistry: parseValue unknown throws",           test_registry_parseValue_throws_unknown_type},
        {"TypeRegistry: validateValue",                       test_registry_validateValue},
        {"TypeRegistry: validateValue unknown returns false", test_registry_validateValue_unknown_type_returns_false},
        {"TypeRegistry: getRegisteredTypesString",            test_registry_getRegisteredTypesString},
        {"TypeRegistry: getRegisteredTypes contains builtins",test_registry_getRegisteredTypes_contains_builtins},
        {"TypeParser<double>: fromString throws on extra chars", test_double_fromString_throws_on_extra_chars},
        {"TypeParser<double>: fromString throws on bad input",   test_double_fromString_throws_on_bad_input},
        {"TypeParser<int>: fromString throws on out of range",   test_int_fromString_throws_on_out_of_range},
        {"TypeParser<bool>: case-insensitive true variants",     test_bool_case_insensitive_true_variants},
        {"TypeParser<bool>: case-insensitive false variants",    test_bool_case_insensitive_false_variants},
        {"TypeParser<vector<double>>: invalid element throws",   test_vector_double_invalid_element_throws},
        {"TypeParser<vector<string>>: elements are trimmed",     test_vector_string_elements_are_trimmed},
        {"TypeParser<vector<int>>: empty string gives empty",    test_vector_int_empty_string_gives_empty_vector},
        {"TypeParser<double>: scientific notation",               test_double_fromString_scientific_notation},
        {"TypeParser<float>: scientific notation",                test_float_fromString_scientific_notation},
        {"TypeParser<unsigned long long>: max value",             test_unsigned_long_long_max_value},
        {"TypeParser<int>: isValid empty string returns false",   test_int_isValid_empty_string_returns_false},
    });
}
