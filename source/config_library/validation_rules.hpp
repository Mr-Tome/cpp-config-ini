#ifndef VALIDATION_RULES_H
#define VALIDATION_RULES_H

#include <functional>
#include <string>
#include "config_reader.hpp"

namespace ValidationRules {

using Rule = std::function<bool(const ConfigValue&)>;

// Core validation rules
Rule greaterThanZero();
Rule greaterThanOrEqualToZero();
Rule betweenValues(double min, double max);
Rule inList(const std::vector<std::string>& validValues);

// Helper function to create custom rules
template<typename T>
Rule custom(std::function<bool(const T&)> customCheck);

// Implementation of custom rule
template<typename T>
Rule custom(std::function<bool(const T&)> customCheck) {
    return [customCheck](const ConfigValue& value) {
        auto typedValue = dynamic_cast<const TypedConfigValue<T>*>(&value);
        return typedValue && customCheck(typedValue->getValue());
    };
}

} // namespace ValidationRules

#endif // VALIDATION_RULES_H
