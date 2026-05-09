#include <algorithm>
#include <sstream>
#include "validation_rules.hpp"
#include "config_value.hpp"

namespace ValidationRules {

bool GreaterThanZero::operator()(const ConfigLib::ConfigValue& value) const 
{
	const auto* numeric = dynamic_cast<const ConfigLib::NumericConfigValue*>(&value);
	return numeric && numeric->toLongDouble() > 0.0;
}

bool GreaterThanOrEqualToZero::operator()(const ConfigLib::ConfigValue& value) const 
{
	const auto* numeric = dynamic_cast<const ConfigLib::NumericConfigValue*>(&value);
	return numeric && numeric->toLongDouble() >= 0.0;
}

bool BetweenValues::operator()(const ConfigLib::ConfigValue& value) const 
{
	const auto* numeric = dynamic_cast<const ConfigLib::NumericConfigValue*>(&value);
	return numeric && numeric->toLongDouble() >= min_ && numeric->toLongDouble() <= max_;
}

std::string BetweenValues::toString() const {
	std::ostringstream oss;
	oss << "Must be between " << min_ << " and " << max_;
	return oss.str();
}

bool InList::operator()(const ConfigLib::ConfigValue& value) const {
	const ConfigLib::TypedConfigValue<std::string>* stringValue = dynamic_cast<const ConfigLib::TypedConfigValue<std::string>*>(&value);
	return stringValue && std::ranges::find(validValues_, stringValue->getValue()) != validValues_.end();
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
std::unique_ptr<Rule> betweenValues(double min, double max) {
	return std::make_unique<BetweenValues>(min, max);
}

std::unique_ptr<Rule> inList(const std::vector<std::string>& validValues) {
	return std::make_unique<InList>(validValues);
}

} //namespace ValidationRules
