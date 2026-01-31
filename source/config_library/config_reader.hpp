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
struct ConfigItem 
{
   const char* name;
   const char* type;
   const char* defaultValue;
   const char* description;
   const ValidationRules::Rule* validationRule;
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

template<typename T>
class TypedConfigValue : public ConfigValue {
public:
	TypedConfigValue(const T& val) : value(val) {}
	
	const T& getValue() const {return value;}
	
	void setValue(const T& val) { value = val; }
	
	std::string toString() const override { return TypeParser<T>::toString(value);}
	
	void fromString(const std::string& str) override {
		value = TypeParser<T>::fromString(str);
	}
	
	std::shared_ptr<ConfigValue> clone() const override {
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

class ConfigSection {
public:

	template<typename T>
	void setValue(const std::string& key, const T& value) 
	{
		std::cout << "ConfigSection::setValue called for key: " << key 
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
			auto error_string = std::string("Exception in ConfigSection::setValue: ") + e.what();
			throw std::runtime_error(error_string);
		} 
		catch (...) 
		{
			auto error_string = std::string("Unknown exception in ConfigSection::setValue");
			throw std::runtime_error(error_string);
		}
	}

    template<typename T>
	T getValue(const std::string& key) const 
	{
		std::cout << "ConfigSection::getValue called for key: " << key 
				  << " with expected type: " << typeid(T).name() << std::endl;
		
		auto it = values.find(key);
		if (it != values.end()) 
		{
			std::cout << "Key found in ConfigSection" << std::endl;
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
			std::cout << "Key not found in ConfigSection" << std::endl;
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

class ConfigReader {
public:
    ConfigReader() = default;
    virtual ~ConfigReader() = default;
    
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


    void saveConfig() const;

    virtual std::string getConfigFilePath() const = 0;
    virtual std::vector<ConfigGen::ConfigSection> getConfigSections() const = 0;
    
    const std::unordered_map<std::string, ConfigSection>& getSections() const { return sections; }
    
protected:
	void initialize();
    void loadConfig();
    void setValidationRules();
    
    void setValidationRule(const std::string& section, const std::string& key, const ValidationRules::Rule* rule);
    
    std::string filepath;
    std::unordered_map<std::string, ConfigSection> sections;
	
private:
    void useDefaultValue(const std::string& section, const std::string& key, const ConfigGen::ConfigItem& item);
    void setValueWithValidation(const std::string& section, const std::string& key, const std::string& value);
	static std::string trim(const std::string& str);
};

void generateConfigFile(const ConfigReader& reader);
} // namespace ConfigLib
