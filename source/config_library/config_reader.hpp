#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include "validation_rules.hpp"
#include "type_parser.hpp"

namespace ConfigLib 
{
namespace ConfigGen 
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
   
   template<typename T>
   static ConfigItem make(const std::string& itemName,
                          const T& defaultVal,
                          const std::string& itemDescription,
                          const ValidationRules::Rule* rule = nullptr)
   {
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
std::string generateConfig(const std::vector<ConfigSection>& sections);
} // namespace ConfigGen

class ConfigValue 
{
public:
	virtual ~ConfigValue() = default;
	virtual std::string toString() const = 0;
	virtual void fromString(const std::string& str) = 0;
	virtual std::shared_ptr<ConfigValue> clone() const = 0;
};

class NumericConfigValue : public ConfigValue
{
public:
	virtual long double toLongDouble() const = 0;
};

//specialization used for non-numeric tpyes, like strings, Color, etc....
template<typename T, typename NotEnabledDummyParameter = void>
class TypedConfigValue : public ConfigValue {
public:
	TypedConfigValue(const T& val) : value(val) {}
	
	const T& getValue() const {return value;}
	void setValue(const T& val) { value = val; }
	
	std::string toString() const override { return TypeParser<T>::toString(value);}
	void fromString(const std::string& str) override {value = TypeParser<T>::fromString(str);}
	
	std::shared_ptr<ConfigValue> clone() const override 
	{
		return std::make_shared<TypedConfigValue<T>>(value);
	}

private:
	T value;
};

//specialization used for numeric types like doubles, ints, etc....
template<typename T>
class TypedConfigValue<T, typename std::enable_if<std::is_arithmetic<T>::value>::type> : public NumericConfigValue 
{
public:
	TypedConfigValue(const T& val) : value(val) {}
	
	const T& getValue() const {return value;}
	void setValue(const T& val) { value = val; }
	
	long double toLongDouble() const override {return static_cast<long double>(value);}
	
	std::string toString() const override { return TypeParser<T>::toString(value);}
	void fromString(const std::string& str) override {value = TypeParser<T>::fromString(str);}
	
	std::shared_ptr<ConfigValue> clone() const override 
	{
		return std::make_shared<TypedConfigValue<T>>(value);
	}

private:
	T value;
};

//free function type registry helper
template<typename T>
std::shared_ptr<ConfigValue> TypeRegistry::createTypedValue(const T& value) 
{
	return std::make_shared<TypedConfigValue<T>>(value);
}

class ConfigSectionStore {
public:

	template<typename T>
	void setValue(const std::string& key, const T& value) 
	{
		std::cout << "ConfigSectionStore::setValue called for key: " << key 
				  << " with type: " << typeid(T).name() << std::endl;
		try 
		{
			auto newValue = std::make_shared<TypedConfigValue<T>>(value);
			// apply the validation runle if it exists
			auto rule_it = validationRules.find(key);
			if (rule_it != validationRules.end() && rule_it->second) {
				if (!(*rule_it->second)(*newValue)) {
					std::cerr << "Validation failed for key: " << key 
							  << ". Using default value." << std::endl;
					throw std::runtime_error("Validation failed for key: " + key);
				}
			}
			
			values[key] = newValue;
			std::cout << "Value set for key: " << key << std::endl;
		} 
		catch (const std::exception& e) 
		{
			auto error_string = std::string("Exception in ConfigSectionStore::setValue: ") + e.what();
			throw std::runtime_error(error_string);
		} 
		catch (...) 
		{
			auto error_string = std::string("Unknown exception in ConfigSectionStore::setValue");
			throw std::runtime_error(error_string);
		}
	}

    template<typename T>
	T getValue(const std::string& key) const 
	{
		std::cout << "ConfigSectionStore::getValue called for key: " << key 
				  << " with expected type: " << typeid(T).name() << std::endl;
		
		auto it = values.find(key);
		if (it != values.end()) 
		{
			std::cout << "Key found in ConfigSectionStore" << std::endl;
			auto typed_value = std::dynamic_pointer_cast<TypedConfigValue<T>>(it->second);
			if (typed_value) 
			{
				std::cout << "Successfully cast to TypedConfigValue<" 
						  << typeid(T).name() << ">" << std::endl;
				return typed_value->getValue();
			} 
			else 
			{
				std::cout << "Failed to cast to TypedConfigValue<" 
						  << typeid(T).name() << ">" << std::endl;
			}
		} 
		else 
		{
			std::cout << "Key not found in ConfigSectionStore" << std::endl;
		}
		throw std::runtime_error("Key not found or type mismatch: " + key);
	}

    void setValidationRule(const std::string& key, const ValidationRules::Rule* rule);
    const std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& getValues() const;
	std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& getValues();

private:
    std::unordered_map<std::string, std::shared_ptr<ConfigValue>> values;
    std::unordered_map<std::string, const ValidationRules::Rule*> validationRules;
};

//owns all data and controls the i/o logic
class ConfigReaderBase {
public:
    ConfigReaderBase() = default;
    virtual ~ConfigReaderBase() = default;
    
	template<typename T>
	T getValue(const std::string& section, const std::string& key) const 
	{
		std::cout << "Attempting to get value for section: " << section 
				  << ", key: " << key << std::endl;
		auto sect_it = sections.find(section);
		if (sect_it != sections.end()) 
		{
			std::cout << "Section found" << std::endl;
			return sect_it->second.getValue<T>(key);
		}
		std::cout << "Section not found" << std::endl;
		throw std::runtime_error("Section not found: " + section);
	}

	template<typename T>
	void setValue(const std::string& section, 
								const std::string& key, const T& value) 
	{
		auto sect_it = sections.find(section);
		if (sect_it == sections.end()) {
			throw std::runtime_error("Attempting to set a Section that was not found in the schema: " + section);
		}
		
		sect_it->second.setValue(key, value);
	}


    void saveConfig(const std::vector<ConfigGen::ConfigSection>& configSections) const;
    
    const std::unordered_map<std::string, ConfigSectionStore>& getSections() const { return sections; }
    
protected:
	void initialize(const std::string& filePath,
					const std::vector<ConfigGen::ConfigSection>& configSections);
    
    void setValidationRule(const std::string& section, const std::string& key, const ValidationRules::Rule* rule);
    
    std::string filepath;
    std::unordered_map<std::string, ConfigSectionStore> sections;
	
private:
    void loadConfig(const std::vector<ConfigGen::ConfigSection>& configSections);
    void setValidationRules(const std::vector<ConfigGen::ConfigSection>& configSections);
    void useDefaultValue(const std::string& section, const std::string& key, const ConfigGen::ConfigItem& item);
	static std::string trim(const std::string& str);
};

//only thing this should be doing is calling derived class initialize and saveConfig
template<typename Derived>
class ConfigReader : public ConfigReaderBase
{
public:
	ConfigReader()
	{
		Derived& d = static_cast<Derived&>(*this);
		initialize(d.getConfigFilePath(), d.getConfigSections());
	}
	
	void saveConfig() const
	{
		const Derived& d = static_cast<const Derived&>(*this);
		ConfigReaderBase::saveConfig(d.getConfigSections());
	}
};
} // namespace ConfigLib
