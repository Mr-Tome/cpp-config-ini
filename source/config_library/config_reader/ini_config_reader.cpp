#include <fstream>
#include <sstream>
#include <typeinfo>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "ini_config_reader.hpp"

namespace ConfigLib 
{
const std::string& iniInstructions()
{
	static const std::string text = R"(
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
	
	return text;
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

	config_content += iniInstructions();

	return config_content;
}
	
    
void generateConfigFileIfNeeded(
	const std::string& filePath,
	const std::vector<ConfigSection>& configSections)
{
	std::ifstream file(filePath);
	
	// TODO (IHT): Update to boost::filesystem::exists(filePath)
	if (file.is_open()) {
		std::cout << "Configuration file already exists. Skipping generation." << std::endl;
		return;
	}


	if (!validateConfig(configSections)) 
	{
		throw std::runtime_error("Invalid configuration detected at runtime");
	}

	std::string configContent = generateConfig(configSections);

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
	
} // namespace ConfigLib
