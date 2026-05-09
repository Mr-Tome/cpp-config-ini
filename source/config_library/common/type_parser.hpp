#pragma once

#include <concepts>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <set>
#include <stdexcept>
#include <type_traits>

namespace ConfigLib
{
    
class ConfigValue; 

// users specialize this template for their custom types.
template<typename T, typename NotEnabledDummyParameter = void>
struct TypeParser
{        
	static T fromString(const std::string& str);
	static std::string toString(const T& value);
	static const char* typeName();
	
	static bool isValid(const std::string& str)
	{
		try
		{
			fromString(str);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
};
    

template<typename T>
concept HasTypeParser =
	requires { TypeParser<T>::fromString(std::string{}); } &&
	requires { TypeParser<T>::toString(std::declval<const T&>()); } &&
	requires { TypeParser<T>::typeName(); };
    

class TypeRegistry {
public:
	static TypeRegistry& instance()
	{
		static TypeRegistry registry; // singleton ftw
		return registry;
	}
	
	using ParserFunc = std::function<std::shared_ptr<ConfigValue>(const std::string&)>;
	using ValidatorFunc = std::function<bool(const std::string&)>;
	
	template<typename T>
	void registerType()
	{
		static_assert(HasTypeParser<T>,
			"Type must have TypeParser specialization with fromString, toString, typeName, and isValid methods. "
			"Please check to ensure all three methods are defined in your specialization!"); // TODO movoe this static_assert to has_type_parser struct.
		
		const std::string name = TypeParser<T>::typeName();
		
		parserMap[name] = [](const std::string& str)
		{
			return createTypedValue(TypeParser<T>::fromString(str));
		};
		
		validatorMap[name] = [](const std::string& str)
		{
			return TypeParser<T>::isValid(str);
		};
		
		typeNames.insert(name);
	}
	
	bool hasType(const std::string& typeName) const
	{
		return typeNames.find(typeName) != typeNames.end();
	}
	
	std::shared_ptr<ConfigValue> parseValue(const std::string& typeName, 
											 const std::string& str) const
	{
		auto it = parserMap.find(typeName);
		if (it != parserMap.end()) {
			return it->second(str);
		}
		throw std::runtime_error("Type not registered: " + typeName + 
			". Did you forget to register it with TypeRegistry?");
	}
	
	bool validateValue(const std::string& typeName, const std::string& str) const
	{
		auto it = validatorMap.find(typeName);
		if (it != validatorMap.end()) {
			return it->second(str);
		}
		return false;
	}
	
	const std::set<std::string>& getRegisteredTypes() const
	{
		return typeNames;
	}
	
	std::string getRegisteredTypesString() const
	{
		std::string result = "Registered types: ";
		bool first = true;
		for (const auto& name : typeNames)
		{
			if (!first) result += ", ";
			result += name;
			first = false;
		}
		return result;
	}
	
private:
	TypeRegistry() = default;
	
	template<typename T>
	static std::shared_ptr<ConfigValue> createTypedValue(const T& value);
	
	std::unordered_map<std::string, ParserFunc> parserMap;
	std::unordered_map<std::string, ValidatorFunc> validatorMap;
	std::set<std::string> typeNames;
};

//auto type registration for the ints, doubles, etc...
template<typename T>
struct LibProvidedType
{
	LibProvidedType()
	{
		TypeRegistry::instance().registerType<T>();
	}
};
    
/////////////////////////////////////////////////////

template<typename Derived>
class ConfigType 
{
public:
	// self-registration mechanism
	struct Registrar 
	{
		Registrar();
	};
	
	static Registrar m_registrar;
	
protected:
	ConfigType() 
	{
		// just touching this during initialization so compiler doesnt
		// optimize away...now can get called before main
		(void)m_registrar;
	}
};

template<typename Derived>
typename ConfigType<Derived>::Registrar ConfigType<Derived>::m_registrar;

//might need to move this below TypeParser on other compilers?
template<typename Derived>
ConfigType<Derived>::Registrar::Registrar() 
{
	TypeRegistry::instance().registerType<Derived>();
}

// C++20 requires-constrained TypeParser specialization for ConfigType-derived types
template<typename Derived>
	requires std::derived_from<Derived, ConfigType<Derived>>
struct TypeParser<Derived, void>
{        
	static Derived fromString(const std::string& str) 
	{
		return Derived::fromString(str);
	}
	
	static std::string toString(const Derived& value) 
	{
		return value.toString();
	}
	
	static const char* typeName() 
	{
		return Derived::typeName();
	}
	
	static bool isValid(const std::string& str) 
	{
		try {
			fromString(str);
			return true;
		} catch (...) {
			return false;
		}
	}
};

} // namespace ConfigLib

