#include "config_reader.hpp"
#include <fstream>
#include <sstream>
#include <typeinfo>
#include <iostream>
#include <stdexcept>
#include <algorithm>


namespace ConfigLib 
{
	namespace ConfigGen 
	{
		// This function can be used for compile-time checks if needed
		constexpr bool validateConfigStructure() 
		{
			// Add any compile-time checks here
			return true;
		}
	
		// Runtime validation of the configuration
		bool validateConfig(const std::vector<ConfigSection>& sections) 
        {
            auto& registry = TypeRegistry::instance();
            bool allValid = true;
            
            for (const auto& section : sections) 
            {
                for (const auto& item : section.items) 
                {
                    // Check if type is registered
                    if (!registry.hasType(item.type)) 
                    {
                        std::cerr << "ERROR: Type '" << item.type << "' for " 
                                  << section.name << "." << item.name 
                                  << " is not registered!" << std::endl;
                        std::cerr << "       " << registry.getRegisteredTypesString() << std::endl;
                        allValid = false;
                    }
                    
                    // validate default value
                    if (!registry.validateValue(item.type, item.defaultValue)) 
                    {
                        std::cerr << "WARNING: Default value '" << item.defaultValue 
                                  << "' is not valid for type '" << item.type 
                                  << "' in " << section.name << "." << item.name << std::endl;
                        allValid = false;
                    }
                }
            }
            
            return allValid;
        }
	
		std::string generateConfig(const std::vector<ConfigSection>& sections) 
		{
			std::string config_content = "# Configuration file generated automatically\n\n";
	
			for (const auto& section : sections) 
			{
				config_content += "[" + section.name + "]\n";
				for (const auto& item : section.items) 
				{
					config_content += std::string(item.name) + " = " + std::string(item.defaultValue)
                        + " # type: " + std::string(item.type)
                        + ", description: " + std::string(item.description);
					if (item.validationRule) 
					{
						config_content += " (validationRule: " + item.validationRule->toString() + ")";
					}
					config_content += "\n";
				}
				config_content += "\n";
			}
	
			config_content += R"(
# Instructions for End Users
# 1. Configuration File Format:
#    * The configuration file is in INI format
#    * Sections are denoted by square brackets: [SectionName]
#    * Key-value pairs are separated by an equals sign: Key = Value
#    * Comments start with a # symbol
# 2. Modifying the Configuration:
#    a. Open the configuration file (e.g., specific_algorithm_config.ini) in a text editor
#    b. Locate the section and key you want to modify
#    c. Change the value after the equals sign
#    d. Save the file
# 3. Example Configuration:
#    [Section1]
#    name1 = 0
#    name2 = 1.0
#    [Section2]
#    name3 = 500.0
#    [Section3]
#    name4 = 10.0
#    name5 = 20.0
#    name1 = 30.0
#    name2 = 1.0,2.0,3.0
# 4. Adding New Values:
#    * You can add new key-value pairs to existing sections
#    * Do not add new sections unless instructed by the developers
# 5. Value Types:
#    * Numbers can be integers or decimals (e.g., 500 or 500.0)
#    * Text should not be enclosed in quotes
#    * Lists are comma-separated (e.g., 1.0,2.0,3.0)
# 6. Validation:
#    * Some values may have validation rules (e.g., must be positive)
#    * If you enter an invalid value, the application will use the default value
# 7. Troubleshooting:
#    * If the application fails to start, check for typos in the configuration file
#    * Ensure all required keys are present
#    * If in doubt, rename or delete the configuration file to reset to defaults
# 8. Best Practices:
#    * Keep a backup of the original configuration file
#    * Document any changes you make for future reference
#    * If you're unsure about a setting, consult the application documentation or contact the developers
# Remember, incorrect configuration can affect the application's performance or cause errors.
# If you're unsure about a setting, it's best to consult with the development team or refer to the application's documentation.
)";
	
			return config_content;
		}
	}
	
	bool ConfigSection::hasKey(const std::string& key) const {
		return values.find(key) != values.end();
	}
	
	void ConfigSection::setValidationRule(const std::string& key, const ValidationRules::Rule* rule) {
        validationRules[key] = rule;
    }
	
	const std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& ConfigSection::getValues() const {
		return values;
	}
	
    std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& ConfigSection::getValues() {
        return values;
    }
    
	ConfigReader::ConfigReader() : filepath("") {
		std::cout << "ConfigReader constructor started" << std::endl;
		//initialize();
		std::cout << "ConfigReader constructor finished" << std::endl;
	}
	
	void ConfigReader::initialize() {
		std::cout << "ConfigReader::initialize started" << std::endl;
		try 
		{
			std::cout << "Calling getConfigFilePath()" << std::endl;
			filepath = getConfigFilePath();
			std::cout << "Config file path: " << filepath << std::endl;
	
			auto configSections = getConfigSections();
			
            std::cout << "Validating configuration schema..." << std::endl;
            if (!ConfigGen::validateConfig(configSections)) 
            {
                throw std::runtime_error("Invalid configuration schema detected. Check error messages above.");
            }
            std::cout << "Schema validation passed." << std::endl;
            
			for (const auto& section : configSections) {
				sections[section.name];  // Creates empty section
			}
	
			// Check if the file exists, if not, generate it
			std::ifstream file(filepath);
			if (!file.is_open()) {
				std::cout << "Config file not found. Generating new file." << std::endl;
				generateConfigFile(*this);
			}
			file.close();
	
			std::cout << "Calling loadConfig()" << std::endl;
			loadConfig();
			std::cout << "Config loaded" << std::endl;
			
			std::cout << "Setting validation rules" << std::endl;
			setValidationRules();
			std::cout << "Validation rules set" << std::endl;
		} 
		catch (const std::exception& e) 
		{
			auto error_string = std::string("Exception in ConfigReader::initialize: ") + e.what();
			throw std::runtime_error(error_string);
		} 
		catch (...) 
		{
			auto error_string = std::string("Unknown exception in ConfigReader::initialize");
			throw std::runtime_error(error_string);
		}
		std::cout << "ConfigReader::initialize finished" << std::endl;
	}	
	
	bool ConfigReader::hasValue(const std::string& section, const std::string& key) const 
	{
		auto sect_it = sections.find(section);
		if (sect_it != sections.end()) {
			return sect_it->second.hasKey(key);
		}
		return false;
	}
	
	void ConfigReader::setValidationRule(const std::string& section, 
										 const std::string& key, 
										 const ValidationRules::Rule* rule) 
	{
        sections[section].setValidationRule(key, rule);
    }
	
	void ConfigReader::setValidationRules() 
	{
        auto configSections = getConfigSections();
        for (const auto& section : configSections) 
        {
            for (const auto& item : section.items) 
            {
                if (item.validationRule) 
                {
                    setValidationRule(section.name, item.name, item.validationRule);
                }
            }
        }
    }
	
	void ConfigReader::loadConfig() 
	{
		std::ifstream file(filepath);
		if (!file.is_open()) 
		{
			std::cerr << "Unable to open file: " << filepath << std::endl;
			return;
		}

		std::string current_section;
		std::string line;
		while (std::getline(file, line)) 
		{
			line = trim(line);
			if (line.empty() || line[0] == '#') continue;

			if (line[0] == '[' && line.back() == ']') 
			{
				current_section = line.substr(1, line.size() - 2);
				continue;
			} 
			
			auto pos = line.find('=');
			
			if(pos == std::string::npos) continue;
			
			std::string key = trim(line.substr(0, pos));
			std::string value = line.substr(pos + 1);
			
			// remove the comments from the value
			size_t commentPos = value.find('#'); //TODO: if a user puts a '#' in a std::string uh oh...
			if (commentPos != std::string::npos) 
			{
				value = value.substr(0, commentPos);
			}
			value = trim(value);
			
			if(current_section.empty()) continue;
			
			auto configSections = getConfigSections();
			for (const auto& section : configSections) 
			{
				if(section.name != current_section) continue;
				
				for (const auto& item : section.items) 
				{
					if (item.name != key) continue;
					try 
					{
						
						auto& registry = TypeRegistry::instance();
						auto parsedValue = registry.parseValue(item.type, value);
						
						//if there's a rule, let's validate against it.
						if (!item.validationRule || (*item.validationRule)(*parsedValue)) 
						{
							sections[current_section].getValues()[key] = parsedValue;
						} 
						else 
						{
							std::cerr << "Validation failed for " << current_section << "." << key 
									  << ". Using default value." << std::endl;
							useDefaultValue(current_section, key, item);
						}
					} 
					catch (const std::exception& e) 
					{
						std::cerr << "Error processing " << current_section << "." << key 
								  << ": " << e.what() << ". Using default value." << std::endl;
						useDefaultValue(current_section, key, item);
					}
					break;
				}
				break;
			}
		}
	}
    	
	void ConfigReader::useDefaultValue(const std::string& section,
									   const std::string& key, 
									   const ConfigGen::ConfigItem& item) 
	{
		auto& registry = TypeRegistry::instance();
		try 
		{
            auto parsedValue = registry.parseValue(item.type, item.defaultValue);
            sections[section].getValues()[key] = parsedValue;
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "ERROR: Default value '" << item.defaultValue << "' for " 
                      << section << "." << key << " failed to parse: " << e.what() << std::endl;
            throw std::runtime_error("Invalid default value in schema for " + section + "." + key);
        }
	}
	
	void ConfigReader::saveConfig() const 
	{
		std::ofstream file(filepath);
		if (!file.is_open()) {
			throw std::runtime_error("Unable to open file for writing: " + filepath);
		}
	
		for (const auto& section : sections) {
			file << "[" << section.first << "]\n";
			for (const auto& value : section.second.getValues()) {
				file << value.first << " = " << value.second->toString() << "\n";
			}
			file << "\n";
		}
	}
	
	std::string ConfigReader::trim(const std::string& str) 
	{
        const auto strBegin = str.find_first_not_of(" \t\r\n");
        if (strBegin == std::string::npos) return "";
        const auto strEnd = str.find_last_not_of(" \t\r\n");
        const auto strRange = strEnd - strBegin + 1;
        return str.substr(strBegin, strRange);
	}
	
	void generateConfigFile(const ConfigReader& reader) 
	{
		std::string filePath = reader.getConfigFilePath();
		std::ifstream file(filePath);
		
		// TODO (IHT): Update to boost::filesystem::exists(filePath)
		if (file.is_open()) {
			std::cout << "Configuration file already exists. Skipping generation." << std::endl;
			return;
		}
		
		auto sections = reader.getConfigSections();
	
		// Perform runtime validation
		if (!ConfigGen::validateConfig(sections)) 
		{
			throw std::runtime_error("Invalid configuration detected at runtime");
		}
		
		// Generate the configuration content
		std::string configContent = ConfigGen::generateConfig(sections);
	
		std::ofstream configFile(filePath);
		if (configFile.is_open()) 
		{
			configFile << configContent;
			configFile.close();
		} 
		else 
		{
			throw std::runtime_error("Unable to open file for writing: " + filePath);
		}
	}
	
	// Compile-time check
	static_assert(ConfigGen::validateConfigStructure(), 
		"Invalid configuration structure detected at compile-time");
	
} // namespace ConfigLib
