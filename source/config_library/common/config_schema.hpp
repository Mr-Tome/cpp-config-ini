#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "type_parser.hpp"
#include "validation_rules.hpp"

namespace ConfigLib
{

// Satisfied by rule types that carry a constexpr check() and expose the runtime singleton.
// Enables ConfigItem::make<T, DefaultVal, RuleType>() to validate defaults at compile time.
template<typename RuleType, typename T>
concept ConstexprValidatable =
    std::is_arithmetic_v<T> &&
    requires(T v) {
        { RuleType::template check<T>(v) } -> std::same_as<bool>;
        { RuleType::rulePtr() }            -> std::convertible_to<const ValidationRules::Rule*>;
    };

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
		
public:
   const std::string name;
   const std::string type;
   const std::string defaultValue;
   const std::string description;
   const ValidationRules::Rule* validationRule;
   const Persistence persistence;

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
		/* brace init forces a compile error on narrowing. Use validated
		(not the potentially moved-from defaultVal) for toString. */
		T validated{ std::forward<U>(defaultVal) };

		return ConfigItem(
		   ForcePrivateConstructorToBeCalled{},
		   itemName,
		   TypeParser<T>::typeName(),
		   TypeParser<T>::toString(validated),
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

   // Compile-time validated make: default value and rule are both template parameters,
   // so the static_assert fires at the call site if the default violates the rule.
   // Also wires up runtime validation via the rule singleton.
   // Usage: ConfigItem::make<int, 8080, ValidationRules::GreaterThanZero>("port", "desc")
   template<typename T, T DefaultVal, typename RuleType>
       requires ConstexprValidatable<RuleType, T>
   [[nodiscard]] static ConfigItem make(const std::string& name, const std::string& desc,
                                         Persistence p = Persistence::Normal)
   {
       static_assert(RuleType::template check<T>(DefaultVal),
                     "Default value violates compile-time validation rule");
       return make<T>(name, DefaultVal, desc, RuleType::rulePtr(), p);
   }
};

struct ConfigSection 
{
   std::string name;
   std::vector<ConfigItem> items;
};

[[nodiscard]] bool validateConfig(const std::vector<ConfigSection>& sections);

[[nodiscard]] std::vector<ConfigSection> mergeDuplicateSections(const std::vector<ConfigSection>& sections);

namespace Internal
{
// sectionName -> set of item names. useful for key-existence checks
using SchemaLookup = std::unordered_map<std::string, std::unordered_set<std::string>>;
[[nodiscard]] SchemaLookup buildSchemaLookup(const std::vector<ConfigSection>& sections);

// sectionName -> itemName -> ConfigItem*.
// Note: ConfigItem* are valid for the lifetime of 'sections'.
using SchemaItemLookup = std::unordered_map<std::string, std::unordered_map<std::string, const ConfigItem*>>;
[[nodiscard]] SchemaItemLookup buildSchemaItemLookup(const std::vector<ConfigSection>& sections);
} // namespace Internal

} // namespace ConfigLib
