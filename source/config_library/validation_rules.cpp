#include "validation_rules.hpp"
#include "config_reader.hpp"
#include <algorithm>
#include <sstream>

namespace ValidationRules {

	bool GreaterThanZero::operator()(const ConfigValue& value) const {
		const TypedConfigValue<double>* doubleValue = dynamic_cast<const TypedConfigValue<double>*>(&value);
		return doubleValue && doubleValue->getValue() > 0;
	}
	
	bool GreaterThanOrEqualToZero::operator()(const ConfigValue& value) const {
		const TypedConfigValue<double>* doubleValue = dynamic_cast<const TypedConfigValue<double>*>(&value);
		return doubleValue && doubleValue->getValue() >= 0;
	}
	
	bool BetweenValues::operator()(const ConfigValue& value) const {
		const TypedConfigValue<double>* doubleValue = dynamic_cast<const TypedConfigValue<double>*>(&value);
		return doubleValue && doubleValue->getValue() >= min_ && doubleValue->getValue() <= max_;
	}
	
	std::string BetweenValues::toString() const {
		std::ostringstream oss;
		oss << "Must be between " << min_ << " and " << max_;
		return oss.str();
	}
	
	bool InList::operator()(const ConfigValue& value) const {
		const TypedConfigValue<std::string>* stringValue = dynamic_cast<const TypedConfigValue<std::string>*>(&value);
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
	BetweenValues betweenValues(double min, double max) {
		return BetweenValues(min, max);
	}
	
	InList inList(const std::vector<std::string>& validValues) {
		return InList(validValues);
	}

} 