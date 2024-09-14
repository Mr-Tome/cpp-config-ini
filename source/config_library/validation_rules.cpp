#include "validation_rules.hpp"
#include "config_reader.hpp"
#include <algorithm>
#include <sstream>

namespace ValidationRules {

	bool GreaterThanZero::operator()(const ConfigLib::ConfigValue& value) const {
        const ConfigLib::TypedConfigValue<double>* doubleValue = dynamic_cast<const ConfigLib::TypedConfigValue<double>*>(&value);
        if (doubleValue) {
            return doubleValue->getValue() > 0;
        }
        const ConfigLib::TypedConfigValue<std::string>* stringValue = dynamic_cast<const ConfigLib::TypedConfigValue<std::string>*>(&value);
        if (stringValue) {
            try {
                return std::stod(stringValue->getValue()) > 0;
            } catch (...) {
                return false;
            }
        }
        return false;
    }
	
	bool GreaterThanOrEqualToZero::operator()(const ConfigLib::ConfigValue& value) const {
		const ConfigLib::TypedConfigValue<double>* doubleValue = dynamic_cast<const ConfigLib::TypedConfigValue<double>*>(&value);
		return doubleValue && doubleValue->getValue() >= 0;
	}
	
	bool BetweenValues::operator()(const ConfigLib::ConfigValue& value) const {
		const ConfigLib::TypedConfigValue<double>* doubleValue = dynamic_cast<const ConfigLib::TypedConfigValue<double>*>(&value);
		if (doubleValue) {
			return doubleValue->getValue() >= min_ && doubleValue->getValue() <= max_;
		}
		const ConfigLib::TypedConfigValue<std::string>* stringValue = dynamic_cast<const ConfigLib::TypedConfigValue<std::string>*>(&value);
		if (stringValue) {
			try {
				double doubleVal = std::stod(stringValue->getValue());
				return doubleVal >= min_ && doubleVal <= max_;
			} catch (...) {
				return false;
			}
		}
		return false;
	}
	
	std::string BetweenValues::toString() const {
		std::ostringstream oss;
		oss << "Must be between " << min_ << " and " << max_;
		return oss.str();
	}
	
	bool InList::operator()(const ConfigLib::ConfigValue& value) const {
		const ConfigLib::TypedConfigValue<std::string>* stringValue = dynamic_cast<const ConfigLib::TypedConfigValue<std::string>*>(&value);
		return stringValue && std::find(validValues_.begin(), validValues_.end(), stringValue->getValue()) != validValues_.end();
	}
	
	std::string InList::toString() const {
		std::ostringstream oss;
		oss << "Must be one of: ";
		for (size_t i = 0; i < validValues_.size(); ++i) {
			if (i > 0) oss << ", ";
			oss << validValues_[i];
		}
		return oss.str();
	}
	
	// Global instances of common rules
	const GreaterThanZero greaterThanZero;
	const GreaterThanOrEqualToZero greaterThanOrEqualToZero;
	
	// Factory functions for rules with parameters
	//BetweenValues betweenValues(double min, double max) {
	//	return BetweenValues(min, max);
	//}
	//
	//InList inList(const std::vector<std::string>& validValues) {
	//	return InList(validValues);
	//}
	Rule* betweenValues(double min, double max) {
        return new BetweenValues(min, max);
    }
	
    Rule* inList(const std::vector<std::string>& validValues) {
        return new InList(validValues);
    }

} 