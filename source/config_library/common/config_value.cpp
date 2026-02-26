#include "config_value.hpp"


namespace ConfigLib 
{
	
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
