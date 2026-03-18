#include "config_value.hpp"


namespace ConfigLib 
{

void ConfigSectionStore::setValueFromParsed(
	const std::string& key,
	std::shared_ptr<ConfigValue> parsedValue)
{
	auto rule_it = validationRules.find(key);
	if(rule_it != validationRules.end() && rule_it->second)
	{
		if(!(*rule_it->second)(*parsedValue))
		{
			throw std::runtime_error(
				"Validation failed for key: " + key + ": "
				+ rule_it->second->toString());
		}
	}
	
	values[key] = std::move(parsedValue);
}

void ConfigSectionStore::setValidationRule(
			const std::string& key,
			const ValidationRules::Rule* rule) 
{
	validationRules[key] = rule;
}

const std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& ConfigSectionStore::getValues() const 
{
	return values;
}

std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& ConfigSectionStore::getValues() 
{
	return values;
}

} // namespace ConfigLib
