#pragma once
#include <string>
#include <vector>
#include "type_parser.hpp"
#include "validation_rules.hpp"

namespace ConfigLib 
{
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
		const ValidationRules::Rule* rule)
	:	name(itemName),
		type(itemType),
		defaultValue(itemDefault),
		description(itemDescription),
		validationRule(rule){};
public:
   const std::string name;
   const std::string type;
   const std::string defaultValue;
   const std::string description;
   const ValidationRules::Rule* validationRule;
   
   //replacing the default stringly typed {} initializer
   ConfigItem(  const std::string& itemName,
				const std::string& itemType,
				const std::string& itemDefaultValue,
				const std::string& itemDescription,
				const ValidationRules::Rule* rule)
	: ConfigItem(
		ForcePrivateConstructorToBeCalled{},
		itemName,
		itemType,
		itemDefaultValue,
		itemDescription,
		rule){};
   
   template<typename T, typename U>
   static ConfigItem make(const std::string& itemName,
                          U&& defaultVal,
                          const std::string& itemDescription,
                          const ValidationRules::Rule* rule = nullptr)
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
		   rule
		);
   }
   
   template<typename T>
   ConfigItem(  const std::string& itemName,
				const T& itemDefaultValue,
				const std::string& itemDescription,
				const ValidationRules::Rule* rule)
	: ConfigItem(this->make<T>(itemName, itemDefaultValue,itemDescription,rule)){};   
};

struct ConfigSection 
{
   std::string name;
   std::vector<ConfigItem> items;
};

bool validateConfig(const std::vector<ConfigSection>& sections);
} // namespace ConfigLib
