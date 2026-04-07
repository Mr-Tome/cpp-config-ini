#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "type_parser.hpp"
#include "validation_rules.hpp"

namespace ConfigLib 
{
//intended to control the visibility/access in various I/O streams
enum class Persistence
{
	Normal,
	Volatile
};
	
class ConfigItem 
{
private:
	//all future constructor paths should come through here so we can avoid the stringly typed members
	struct ForcePrivateConstructorToBeCalled{};
	ConfigItem(
		ForcePrivateConstructorToBeCalled,
		const std::string& itemName,
		const std::string& itemType, 
		const std::string& itemDefault,
		const std::string& itemDescription,
		const ValidationRules::Rule* rule,
		Persistence p)
	:	name(itemName),
		type(itemType),
		defaultValue(itemDefault),
		description(itemDescription),
		validationRule(rule),
		persistence(p){}
		
public:
   const std::string name;
   const std::string type;
   const std::string defaultValue;
   const std::string description;
   const ValidationRules::Rule* validationRule;
   const Persistence persistence;
   
   //replacing the default stringly typed {} initializer
   ConfigItem(  const std::string& itemName,
				const std::string& itemType,
				const std::string& itemDefaultValue,
				const std::string& itemDescription,
				const ValidationRules::Rule* rule,
				Persistence p = Persistence::Normal)
	: ConfigItem(
		ForcePrivateConstructorToBeCalled{},
		itemName,
		itemType,
		itemDefaultValue,
		itemDescription,
		rule,
		p){}
   
   template<typename T, typename U>
   static ConfigItem make(const std::string& itemName,
                          U&& defaultVal,
                          const std::string& itemDescription,
                          const ValidationRules::Rule* rule = nullptr,
                          Persistence p = Persistence::Normal)
   {
		/* brace init with a lambda forces a compile error on narrowing.
		e.g., make<int>("x", 1.3, ...) fails on check({...}) because cannot narrow double to int.
		* */
		T validated{ std::forward<U>(defaultVal) };
		(void)validated;
	   
		return ConfigItem(
		   ForcePrivateConstructorToBeCalled{},
		   itemName,
		   TypeParser<T>::typeName(),
		   TypeParser<T>::toString(defaultVal),
		   itemDescription,
		   rule, 
		   p
		);
   }
   
   template<typename T>
   ConfigItem(  const std::string& itemName,
				const T& itemDefaultValue,
				const std::string& itemDescription,
				const ValidationRules::Rule* rule,
				Persistence p = Persistence::Normal)
	: ConfigItem(this->make<T>(itemName, itemDefaultValue,itemDescription,rule,p)){}
};

struct ConfigSection 
{
   std::string name;
   std::vector<ConfigItem> items;
};

bool validateConfig(const std::vector<ConfigSection>& sections);

std::vector<ConfigSection> mergeDuplicateSections(const std::vector<ConfigSection>& sections);

// sectionName -> set of item names. useful for key-existence checks
using SchemaLookup = std::unordered_map<std::string, std::unordered_set<std::string>>;
SchemaLookup buildSchemaLookup(const std::vector<ConfigSection>& sections);

// sectionName -> itemName -> ConfigItem*. 
// useful where the full item metadata is needed
// Note: ConfigItem* are valid for the lifetime of 'sections'.
using SchemaItemLookup = std::unordered_map<std::string, std::unordered_map<std::string, const ConfigItem*>>;
SchemaItemLookup buildSchemaItemLookup(const std::vector<ConfigSection>& sections);

} // namespace ConfigLib
