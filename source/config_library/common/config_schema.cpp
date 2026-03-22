#include <unordered_map>
#include <iostream>
#include <algorithm>
#include "config_schema.hpp"
#include "config_value.hpp"


namespace ConfigLib 
{	
// function can be used for compile-time checks if needed
constexpr bool validateConfigStructure() 
{
	// i can't do it yet, but compile-time checks here..
	return true;
}

// unlike the function above, this is the runtime validation of the configuration
bool validateConfig(const std::vector<ConfigSection>& sections) 
{
	auto& registry = TypeRegistry::instance();
	bool allValid = true;
	
	for (const auto& section : sections) 
	{
		for (const auto& item : section.items) 
		{
			// Check if type is registered
			if (!registry.hasType(item.type)) 
			{
				std::cerr << "ERROR: Type '" << item.type << "' for " 
						  << section.name << "." << item.name 
						  << " is not registered!" << std::endl;
				std::cerr << "       " << registry.getRegisteredTypesString() << std::endl;
				allValid = false;
			}
			
			// validate default value
			if (!registry.validateValue(item.type, item.defaultValue)) 
			{
				std::cerr << "WARNING: Default value '" << item.defaultValue 
						  << "' is not valid for type '" << item.type 
						  << "' in " << section.name << "." << item.name << std::endl;
				allValid = false;
			}
		}
	}
	
	return allValid;
}

std::vector<ConfigSection> mergeDuplicateSections(const std::vector<ConfigSection>& sections)
{
	std::vector<ConfigSection> merged;
	std::unordered_map<std::string, size_t> indexByName;
	
	for (const auto& section : sections)
	{
		auto it = indexByName.find(section.name);
		if (it == indexByName.end())
		{
			indexByName[section.name] = merged.size();
			merged.push_back(section);
		}
		else
		{
			ConfigSection& target = merged[it->second];
			for (const auto& item : section.items)
			{
				const bool duplicate = std::any_of(
					target.items.begin(), target.items.end(),
					[&](const ConfigItem& existing){ return existing.name == item.name; });
				if (duplicate)
					throw std::runtime_error(
						"Duplicate ConfigItem '" + section.name + "." + item.name
						+ "' found when merging sections with the same name.");
				target.items.push_back(item);
			}
		}
	}
	return merged;
}

// compile-time check
static_assert(validateConfigStructure(), 
	"Invalid configuration structure detected at compile-time");
		
} // namespace ConfigLib
