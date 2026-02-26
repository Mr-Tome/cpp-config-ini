#include <unordered_map>
#include <iostream>
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


// compile-time check
static_assert(validateConfigStructure(), 
	"Invalid configuration structure detected at compile-time");
		
} // namespace ConfigLib
