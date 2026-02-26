#pragma once
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <iostream>
#include "type_parser.hpp"
#include "validation_rules.hpp"

namespace ConfigLib 
{

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

} // namespace ConfigLib
