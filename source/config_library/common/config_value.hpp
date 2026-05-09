#pragma once
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "config_logger.hpp"
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
  requires std::is_arithmetic_v<T>
class TypedConfigValue<T, void> : public NumericConfigValue
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
		Internal::log(std::string("ConfigSectionStore::setValue called for key: ") + key
				  + " with type: " + typeid(T).name());
		try 
		{
			auto newValue = std::make_shared<TypedConfigValue<T>>(value);
			// apply the validation runle if it exists
			auto rule_it = validationRules.find(key);
			if (rule_it != validationRules.end() && rule_it->second) {
				if (!(*rule_it->second)(*newValue)) {
					Internal::log("Validation failed for key: " + key + ". Using default value.");
					throw std::runtime_error("Validation failed for key: " + key);
				}
			}
			
			values[key] = newValue;
			Internal::log("Value set for key: " + key);
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
		Internal::log(std::string("ConfigSectionStore::getValue called for key: ") + key
				  + " with expected type: " + typeid(T).name());
		
		auto it = values.find(key);
		if (it != values.end()) 
		{
			Internal::log("Key found in ConfigSectionStore");
			auto typed_value = std::dynamic_pointer_cast<TypedConfigValue<T>>(it->second);
			if (typed_value) 
			{
				Internal::log(std::string("Successfully cast to TypedConfigValue<")
						  + typeid(T).name() + ">");
				return typed_value->getValue();
			} 
			else 
			{
				Internal::log(std::string("Failed to cast to TypedConfigValue<")
						  + typeid(T).name() + ">");
			}
		} 
		else 
		{
			Internal::log("Key not found in ConfigSectionStore");
		}
		throw std::runtime_error("Key not found or type mismatch: " + key);
	}

    void setValidationRule(const std::string& key, const ValidationRules::Rule* rule);
    void setValueFromParsed(const std::string& key, std::shared_ptr<ConfigValue> parsedValue);
    
    const std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& getValues() const;
	std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& getValues();

private:
    std::unordered_map<std::string, std::shared_ptr<ConfigValue>> values;
    std::unordered_map<std::string, const ValidationRules::Rule*> validationRules;
};

} // namespace ConfigLib
