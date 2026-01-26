#include "type_parser.hpp"
#include "config_reader.hpp"
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace ConfigLib 
{
    
    // helper function for trimming strings
    namespace 
    {
        std::string trim(const std::string& str) 
        {
            const auto strBegin = str.find_first_not_of(" \t\r\n");
            if (strBegin == std::string::npos) return "";
            const auto strEnd = str.find_last_not_of(" \t\r\n");
            const auto strRange = strEnd - strBegin + 1;
            return str.substr(strBegin, strRange);
        }
    }
    
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
    
    template<>
	double TypeParser<double>::fromString(const std::string& str) 
	{
		try 
		{
			size_t pos;
			double value = std::stod(str, &pos);
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
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		
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
			token = trim(token);
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
			token = trim(token);
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
			token = trim(token);
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
        TypeRegistrar<int> registerInt;
        TypeRegistrar<double> registerDouble;
        TypeRegistrar<bool> registerBool;
        TypeRegistrar<std::string> registerString;
        TypeRegistrar<std::vector<int>> registerVecInt;
        TypeRegistrar<std::vector<double>> registerVecDouble;
        TypeRegistrar<std::vector<std::string>> registerVecString;
    }

} // namespace ConfigLib
