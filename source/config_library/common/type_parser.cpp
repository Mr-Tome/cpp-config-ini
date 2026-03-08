#include <sstream>
#include <algorithm>
#include <stdexcept>
#include "type_parser.hpp"
#include "config_lib_internal_utility.hpp"
#include "config_value.hpp"

namespace ConfigLib 
{    
// library-provided / built-in Type Parser Specializations 
template<>
int TypeParser<int>::fromString(const std::string& str) 
{
	try 
	{
		size_t pos;
		int value = std::stoi(str, &pos);
		if (pos != str.length()) 
		{
			throw std::invalid_argument("Extra characters after integer");
		}
		return value;
	} 
	catch (const std::invalid_argument& e) 
	{
		throw std::runtime_error("Cannot parse '" + str + "' as int: " + e.what());
	} 
	catch (const std::out_of_range& e) 
	{
		throw std::runtime_error("Value '" + str + "' out of range for int: " + e.what());
	}
}
template<>
std::string TypeParser<int>::toString(const int& value) 
{
	return std::to_string(value);
}
template<>
const char* TypeParser<int>::typeName() 
{
	return "int";
}

/////////double
template<>
double TypeParser<double>::fromString(const std::string& str) 
{
	try 
	{
		size_t pos;
		auto value = std::stod(str, &pos);
		if (pos != str.length()) 
		{
			throw std::invalid_argument("Extra characters after number");
		}
		return value;
	} 
	catch (const std::invalid_argument& e) 
	{
		throw std::runtime_error("Cannot parse '" + str + "' as double: " + e.what());
	} 
	catch (const std::out_of_range& e) 
	{
		throw std::runtime_error("Value '" + str + "' out of range for double: " + e.what());
	}
}

template<>
std::string TypeParser<double>::toString(const double& value) 
{
	return std::to_string(value);
}

template<>
const char* TypeParser<double>::typeName() 
{
	return "double";
}
/////////

/////////long double
template<>
long double TypeParser<long double>::fromString(const std::string& str) 
{
	try 
	{
		size_t pos;
		auto value = std::stold(str, &pos);
		if (pos != str.length()) 
		{
			throw std::invalid_argument("Extra characters after number");
		}
		return value;
	} 
	catch (const std::invalid_argument& e) 
	{
		throw std::runtime_error("Cannot parse '" + str + "' as long double: " + e.what());
	} 
	catch (const std::out_of_range& e) 
	{
		throw std::runtime_error("Value '" + str + "' out of range for long double: " + e.what());
	}
}

template<>
std::string TypeParser<long double>::toString(const long double& value) 
{
	return std::to_string(value);
}

template<>
const char* TypeParser<long double>::typeName() 
{
	return "long double";
}
/////////

//////////float
template<>
float TypeParser<float>::fromString(const std::string& str) 
{
	try 
	{
		size_t pos;
		auto value = std::stof(str, &pos);
		if (pos != str.length()) 
		{
			throw std::invalid_argument("Extra characters after number");
		}
		return value;
	} 
	catch (const std::invalid_argument& e) 
	{
		throw std::runtime_error("Cannot parse '" + str + "' as float: " + e.what());
	} 
	catch (const std::out_of_range& e) 
	{
		throw std::runtime_error("Value '" + str + "' out of range for float: " + e.what());
	}
}

template<>
std::string TypeParser<float>::toString(const float& value) 
{
	return std::to_string(value);
}

template<>
const char* TypeParser<float>::typeName() 
{
	return "float";
}
//////////

//////////// long
template<>
long TypeParser<long>::fromString(const std::string& str) 
{
	try 
	{
		size_t pos;
		auto value = std::stol(str, &pos);
		if (pos != str.length()) 
		{
			throw std::invalid_argument("Extra characters after number");
		}
		return value;
	} 
	catch (const std::invalid_argument& e) 
	{
		throw std::runtime_error("Cannot parse '" + str + "' as long: " + e.what());
	} 
	catch (const std::out_of_range& e) 
	{
		throw std::runtime_error("Value '" + str + "' out of range for long: " + e.what());
	}
}
template<>
std::string TypeParser<long>::toString(const long& value) 
{
	return std::to_string(value);
}
template<>
const char* TypeParser<long>::typeName() 
{
	return "long";
}
////////////

//////////// long long
template<>
long long TypeParser<long long>::fromString(const std::string& str) 
{
	try 
	{
		size_t pos;
		auto value = std::stoll(str, &pos);
		if (pos != str.length()) 
		{
			throw std::invalid_argument("Extra characters after number");
		}
		return value;
	} 
	catch (const std::invalid_argument& e) 
	{
		throw std::runtime_error("Cannot parse '" + str + "' as long long: " + e.what());
	} 
	catch (const std::out_of_range& e) 
	{
		throw std::runtime_error("Value '" + str + "' out of range for long long: " + e.what());
	}
}
template<>
std::string TypeParser<long long>::toString(const long long& value) 
{
	return std::to_string(value);
}
template<>
const char* TypeParser<long long>::typeName() 
{
	return "long long";
}
////////////

//////////// unsigned int
template<>
unsigned int TypeParser<unsigned int>::fromString(const std::string& str) 
{
	try 
	{
		size_t pos;
		auto value = static_cast<unsigned int>(std::stoul(str, &pos));
		if (pos != str.length()) 
		{
			throw std::invalid_argument("Extra characters after number");
		}
		return value;
	} 
	catch (const std::invalid_argument& e) 
	{
		throw std::runtime_error("Cannot parse '" + str + "' as unsigned int: " + e.what());
	} 
	catch (const std::out_of_range& e) 
	{
		throw std::runtime_error("Value '" + str + "' out of range for unsigned int: " + e.what());
	}
}
template<>
std::string TypeParser<unsigned int>::toString(const unsigned int& value) 
{
	return std::to_string(value);
}
template<>
const char* TypeParser<unsigned int>::typeName() 
{
	return "unsigned int";
}
////////////

//////////// unsigned long
template<>
unsigned long TypeParser<unsigned long>::fromString(const std::string& str) 
{
	try 
	{
		size_t pos;
		auto value = std::stoul(str, &pos);
		if (pos != str.length()) 
		{
			throw std::invalid_argument("Extra characters after number");
		}
		return value;
	} 
	catch (const std::invalid_argument& e) 
	{
		throw std::runtime_error("Cannot parse '" + str + "' as unsigned long: " + e.what());
	} 
	catch (const std::out_of_range& e) 
	{
		throw std::runtime_error("Value '" + str + "' out of range for unsigned long: " + e.what());
	}
}
template<>
std::string TypeParser<unsigned long>::toString(const unsigned long& value) 
{
	return std::to_string(value);
}
template<>
const char* TypeParser<unsigned long>::typeName() 
{
	return "unsigned long";
}
////////////


//////////// unsigned long long
template<>
unsigned long long TypeParser<unsigned long long>::fromString(const std::string& str) 
{
	try 
	{
		size_t pos;
		auto value = std::stoull(str, &pos);
		if (pos != str.length()) 
		{
			throw std::invalid_argument("Extra characters after number");
		}
		return value;
	} 
	catch (const std::invalid_argument& e) 
	{
		throw std::runtime_error("Cannot parse '" + str + "' as unsigned long long: " + e.what());
	} 
	catch (const std::out_of_range& e) 
	{
		throw std::runtime_error("Value '" + str + "' out of range for unsigned long long: " + e.what());
	}
}
template<>
std::string TypeParser<unsigned long long>::toString(const unsigned long long& value) 
{
	return std::to_string(value);
}
template<>
const char* TypeParser<unsigned long long>::typeName() 
{
	return "unsigned long long";
}
////////////

template<>
std::string TypeParser<std::string>::fromString(const std::string& str) 
{
	return str;
}
template<>
std::string TypeParser<std::string>::toString(const std::string& value) 
{
	return value;
}
template<>
const char* TypeParser<std::string>::typeName() 
{
	return "string";
}

template<>
bool TypeParser<bool>::fromString(const std::string& str) 
{
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(), 
	[](unsigned char c){return static_cast<char>(std::tolower(c));});
	
	if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") 
	{
		return true;
	} 
	else if (lower == "false" || lower == "0" || lower == "no" || lower == "off") 
	{
		return false;
	} 
	else 
	{
		throw std::runtime_error("Cannot parse '" + str + 
			"' as bool. Valid values: true/false, 1/0, yes/no, on/off");
	}
}

template<>
std::string TypeParser<bool>::toString(const bool& value) 
{
	return value ? "true" : "false";
}

template<>
const char* TypeParser<bool>::typeName() 
{
	return "bool";
}

template<>
std::vector<double> TypeParser<std::vector<double>>::fromString(const std::string& str) 
{
	std::vector<double> result;
	std::istringstream iss(str);
	std::string token;
	
	while (std::getline(iss, token, ',')) 
	{
		token = ConfigLib::Internal::trim(token);
		if (token.empty()) continue;
		
		try 
		{
			size_t pos;
			double value = std::stod(token, &pos);
			if (pos != token.length()) 
			{
				throw std::invalid_argument("Extra characters in number");
			}
			result.push_back(value);
		} 
		catch (const std::exception& e) 
		{
			throw std::runtime_error("Cannot parse vector element '" + token + 
				"' as double: " + e.what());
		}
	}
	
	return result;
}

template<>
std::string TypeParser<std::vector<double>>::toString(const std::vector<double>& value) 
{
	std::ostringstream oss;
	for (size_t i = 0; i < value.size(); ++i) 
	{
		if (i > 0) oss << ",";
		oss << value[i];
	}
	return oss.str();
}

template<>
const char* TypeParser<std::vector<double>>::typeName() 
{
	return "vector<double>";
}


template<>
std::vector<int> TypeParser<std::vector<int>>::fromString(const std::string& str) 
{
	std::vector<int> result;
	std::istringstream iss(str);
	std::string token;
	
	while (std::getline(iss, token, ',')) 
	{
		token = ConfigLib::Internal::trim(token);
		if (token.empty()) continue;
		
		try 
		{
			size_t pos;
			int value = std::stoi(token, &pos);
			if (pos != token.length()) 
			{
				throw std::invalid_argument("Extra characters in number");
			}
			result.push_back(value);
		} 
		catch (const std::exception& e) 
		{
			throw std::runtime_error("Cannot parse vector element '" + token + 
				"' as int: " + e.what());
		}
	}
	
	return result;
}

template<>
std::string TypeParser<std::vector<int>>::toString(const std::vector<int>& value) 
{
	std::ostringstream oss;
	for (size_t i = 0; i < value.size(); ++i) 
	{
		if (i > 0) oss << ",";
		oss << value[i];
	}
	return oss.str();
}

template<>
const char* TypeParser<std::vector<int>>::typeName() 
{
	return "vector<int>";
}


template<>
std::vector<std::string> TypeParser<std::vector<std::string>>::fromString(const std::string& str) 
{
	std::vector<std::string> result;
	std::istringstream iss(str);
	std::string token;
	
	while (std::getline(iss, token, ',')) 
	{
		token = ConfigLib::Internal::trim(token);
		if (!token.empty()) 
		{
			result.push_back(token);
		}
	}
	
	return result;
}

template<>
std::string TypeParser<std::vector<std::string>>::toString(const std::vector<std::string>& value) 
{
	std::ostringstream oss;
	for (size_t i = 0; i < value.size(); ++i) 
	{
		if (i > 0) oss << ",";
		oss << value[i];
	}
	return oss.str();
}

template<>
const char* TypeParser<std::vector<std::string>>::typeName() {
	return "vector<string>";
}

// registration of library-provided / built-in types
 
namespace 
{
	LibProvidedType<int> registerInt;
	LibProvidedType<double> registerDouble;
	LibProvidedType<bool> registerBool;
	LibProvidedType<std::string> registerString;
	LibProvidedType<std::vector<int>> registerVecInt;
	LibProvidedType<std::vector<double>> registerVecDouble;
	LibProvidedType<std::vector<std::string>> registerVecString;
	
	LibProvidedType<float>             registerFloat;
	LibProvidedType<long>              registerLong;
	LibProvidedType<long long>         registerLongLong;
	LibProvidedType<unsigned int>      registerUInt;
	LibProvidedType<unsigned long>     registerULong;
	LibProvidedType<unsigned long long>registerULongLong;
	LibProvidedType<long double>       registerLongDouble;
	//TODO (IHT 2026.02.22): make the vectoor versions of these.
}

} // namespace ConfigLib
