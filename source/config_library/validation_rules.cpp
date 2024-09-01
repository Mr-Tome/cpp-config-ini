#include "validation_rules.hpp"
#include <algorithm>

namespace ValidationRules {

Rule greaterThanZero() {
    return [](const ConfigValue& value) {
        auto doubleValue = dynamic_cast<const TypedConfigValue<double>*>(&value);
        return doubleValue && doubleValue->getValue() > 0;
    };
}

Rule greaterThanOrEqualToZero() {
    return [](const ConfigValue& value) {
        auto doubleValue = dynamic_cast<const TypedConfigValue<double>*>(&value);
        return doubleValue && doubleValue->getValue() >= 0;
    };
}

Rule betweenValues(double min, double max) {
    return [min, max](const ConfigValue& value) {
        auto doubleValue = dynamic_cast<const TypedConfigValue<double>*>(&value);
        return doubleValue && doubleValue->getValue() >= min && doubleValue->getValue() <= max;
    };
}

Rule inList(const std::vector<std::string>& validValues) {
    return [validValues](const ConfigValue& value) {
        auto stringValue = dynamic_cast<const TypedConfigValue<std::string>*>(&value);
        return stringValue && std::find(validValues.begin(), validValues.end(), stringValue->getValue()) != validValues.end();
    };
}

} // namespace ValidationRules
