#include "test_framework.hpp"
#include "config_library/common/config_value.hpp"
#include "config_library/common/type_parser.hpp"
#include "config_library/common/config_schema.hpp"
#include <stdexcept>

// A user-defined type registered via ConfigType<T>
enum class Color { Red, Green, Blue };

class ColorType : public ConfigLib::ConfigType<ColorType>
{
public:
    Color value;

    explicit ColorType(Color c = Color::Red) : value(c) {}

    static ColorType fromString(const std::string& str)
    {
        if (str == "red")   return ColorType(Color::Red);
        if (str == "green") return ColorType(Color::Green);
        if (str == "blue")  return ColorType(Color::Blue);
        throw std::runtime_error("Invalid Color: " + str);
    }

    std::string toString() const
    {
        switch (value)
        {
            case Color::Red:   return "red";
            case Color::Green: return "green";
            case Color::Blue:  return "blue";
        }
        return "red";
    }

    static const char* typeName() { return "Color"; }
};

static bool test_configType_auto_registers()
{
    REQUIRE(ConfigLib::TypeRegistry::instance().hasType("Color") == true);
    return true;
}

static bool test_getRegisteredTypes_includes_custom_type()
{
    const auto& types = ConfigLib::TypeRegistry::instance().getRegisteredTypes();
    REQUIRE(types.count("Color") > static_cast<std::size_t>(0));
    return true;
}

static bool test_registerType_is_idempotent()
{
    ConfigLib::TypeRegistry::instance().registerType<ColorType>();
    REQUIRE(ConfigLib::TypeRegistry::instance().hasType("Color") == true);
    return true;
}

static bool test_parseValue_roundtrip()
{
    auto val = ConfigLib::TypeRegistry::instance().parseValue("Color", "green");
    REQUIRE(val != nullptr);
    REQUIRE_EQ(val->toString(), std::string("green"));
    return true;
}

static bool test_validateValue_valid()
{
    REQUIRE(ConfigLib::TypeRegistry::instance().validateValue("Color", "red") == true);
    return true;
}

static bool test_validateValue_invalid()
{
    REQUIRE(ConfigLib::TypeRegistry::instance().validateValue("Color", "pink") == false);
    return true;
}

static bool test_typeParser_fromString()
{
    ColorType c = ConfigLib::TypeParser<ColorType>::fromString("blue");
    REQUIRE(c.value == Color::Blue);
    return true;
}

static bool test_typeParser_toString()
{
    ColorType c(Color::Green);
    REQUIRE_EQ(ConfigLib::TypeParser<ColorType>::toString(c), std::string("green"));
    return true;
}

static bool test_typeParser_typeName()
{
    REQUIRE_EQ(std::string(ConfigLib::TypeParser<ColorType>::typeName()), std::string("Color"));
    return true;
}

static bool test_typeParser_isValid_true()
{
    REQUIRE(ConfigLib::TypeParser<ColorType>::isValid("red") == true);
    return true;
}

static bool test_typeParser_isValid_false()
{
    REQUIRE(ConfigLib::TypeParser<ColorType>::isValid("purple") == false);
    return true;
}

static bool test_configItem_make_with_custom_type()
{
    ColorType defaultColor(Color::Green);
    auto item = ConfigLib::ConfigItem::make<ColorType>(
        "color", defaultColor, "background color");
    REQUIRE_EQ(item.type,         std::string("Color"));
    REQUIRE_EQ(item.defaultValue, std::string("green"));
    return true;
}

static bool test_validateConfig_accepts_custom_type()
{
    ColorType defaultColor(Color::Blue);
    std::vector<ConfigLib::ConfigSection> sections = {{
        "palette",
        { ConfigLib::ConfigItem::make<ColorType>("bg", defaultColor, "background") }
    }};
    REQUIRE(ConfigLib::validateConfig(sections) == true);
    return true;
}

int main()
{
    return runTests({
        {"ConfigType: auto-registers in TypeRegistry",         test_configType_auto_registers},
        {"TypeRegistry::getRegisteredTypes includes custom",   test_getRegisteredTypes_includes_custom_type},
        {"TypeRegistry::registerType is idempotent",           test_registerType_is_idempotent},
        {"TypeRegistry::parseValue round-trip",                test_parseValue_roundtrip},
        {"TypeRegistry::validateValue valid",                  test_validateValue_valid},
        {"TypeRegistry::validateValue invalid",                test_validateValue_invalid},
        {"TypeParser<custom>::fromString",                     test_typeParser_fromString},
        {"TypeParser<custom>::toString",                       test_typeParser_toString},
        {"TypeParser<custom>::typeName",                       test_typeParser_typeName},
        {"TypeParser<custom>::isValid true",                   test_typeParser_isValid_true},
        {"TypeParser<custom>::isValid false",                  test_typeParser_isValid_false},
        {"ConfigItem::make with custom type",                  test_configItem_make_with_custom_type},
        {"validateConfig accepts custom type",                 test_validateConfig_accepts_custom_type},
    });
}
