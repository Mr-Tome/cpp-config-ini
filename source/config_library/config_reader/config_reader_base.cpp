#include <fstream>
#include <sstream>
#include <typeinfo>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "config_reader_base.hpp"
#include "../common/config_lib_internal_utility.hpp"

namespace ConfigLib 
{
									
	
void ConfigReaderBase::initialize(
			const std::string& filePath,
			const std::vector<ConfigSection>& configSections) 
{
	std::cout << "ConfigReader::initialize started" << std::endl;
	try 
	{
		this->filepath = filePath;
		std::cout << "Config file path: " << filepath << std::endl;

		std::cout << "Validating configuration schema..." << std::endl;
		if (!validateConfig(configSections)) 
		{
			throw std::runtime_error("Invalid configuration schema detected. Check error messages above.");
		}
		std::cout << "Schema validation passed." << std::endl;
		
		for (const auto& section : configSections) {
			sections[section.name];  // intentionally creating empty sections
		}

		std::cout << "Calling loadConfig()" << std::endl;
		loadConfig(configSections);
		std::cout << "Config loaded" << std::endl;
		
		std::cout << "Setting validation rules" << std::endl;
		setValidationRules(configSections);
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
void ConfigReaderBase::assertNoVolatileFieldsInINIOnlyReader(const std::vector<ConfigSection>& configSections) const
{
	for (const auto& section : configSections)
		for (const auto& item : section.items)
			if (item.persistence == Persistence::Volatile)
				throw std::runtime_error(
					"ConfigItem '" + section.name + "." + item.name + "' is marked "
					"Persistence::Volatile but ConfigLib::CLI is not in your ConfigReader "
					"type list. Volatile fields must be supplied via CLI every run — they "
					"have no meaning without it. Add ConfigLib::CLI or change the field "
					"to Persistence::Normal.");
}
	
void ConfigReaderBase::initializeForCLI(
			const std::vector<ConfigSection>& configSections) 
{
	std::cout << "ConfigReader::initializeForCLI started" << std::endl;
	try 
	{
		std::cout << "Validating configuration schema..." << std::endl;
		if (!validateConfig(configSections)) 
		{
			throw std::runtime_error("Invalid configuration schema detected. Check error messages above.");
		}
		std::cout << "Schema validation passed." << std::endl;
		
		for (const auto& section : configSections) {
			sections[section.name];  // intentionally creating empty sections
			for (const auto& item : section.items)
            {
                useDefaultValue(section.name, item.name, item);
            }
		}
		
		std::cout << "Setting validation rules" << std::endl;
		setValidationRules(configSections);
		std::cout << "Validation rules set" << std::endl;
	} 
	catch (const std::exception& e) 
	{
		auto error_string = std::string("Exception in ConfigReader::initializeForCLI: ") + e.what();
		throw std::runtime_error(error_string);
	} 
	catch (...) 
	{
		auto error_string = std::string("Unknown exception in ConfigReader::initializeForCLI");
		throw std::runtime_error(error_string);
	}
	std::cout << "ConfigReader::initializeForCLI finished" << std::endl;
}	

void ConfigReaderBase::setValidationRule(const std::string& section, 
									 const std::string& key, 
									 const ValidationRules::Rule* rule) 
{
	sections[section].setValidationRule(key, rule);
}
	
void ConfigReaderBase::setValidationRules(
		const std::vector<ConfigSection>& configSections) 
{
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
	
void ConfigReaderBase::loadConfig(
		const std::vector<ConfigSection>& configSections) 
{
	std::ifstream file(filepath);
	if (!file.is_open()) 
	{
		std::cerr << "Unable to open file: " << filepath << std::endl;
		return;
	}

	std::string current_section;
	std::string line;
	
	std::unordered_map<std::string,
		std::unordered_map<std::string, const ConfigItem*>> schemaLookup;
		
	for (const auto& section : configSections)
		for (const auto& item : section.items)
			schemaLookup[section.name][item.name] = &item;
			
	while (std::getline(file, line)) 
	{
		line = ConfigLib::Internal::trim(line);
		if (line.empty() || line[0] == '#') continue;

		if (line[0] == '[' && line.back() == ']') 
		{
			current_section = line.substr(1, line.size() - 2);
			continue;
		} 
		
		auto pos = line.find('=');
		
		if(pos == std::string::npos) continue;
		
		std::string key = ConfigLib::Internal::trim(line.substr(0, pos));
		std::string value = line.substr(pos + 1);
		
		// remove the comments from the value
		size_t commentPos = value.find('#'); //TODO: if a user puts a '#' in a std::string uh oh...
		if (commentPos != std::string::npos) 
		{
			value = value.substr(0, commentPos);
		}
		value = ConfigLib::Internal::trim(value);
		
		if(current_section.empty()) continue;
		
		auto sectionIt = schemaLookup.find(current_section);
		if (sectionIt == schemaLookup.end()) continue;
		
		auto itemIt = sectionIt->second.find(key);
		if (itemIt == sectionIt->second.end()) continue;
		const ConfigItem& item = *itemIt->second;
		
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
	}
}
    	
void ConfigReaderBase::useDefaultValue(const std::string& section,
								   const std::string& key, 
								   const ConfigItem& item) 
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
	
void ConfigReaderBase::saveConfig(
		const std::vector<ConfigSection>& configSections,
		const std::string& instructions_footer) const 
{
	std::cout << "Saving the current configuration to: " << this->filepath<< std::endl;
	std::ofstream file(filepath);
	if (!file.is_open()) 
	{
		throw std::runtime_error("Unable to open file for writing: " + filepath);
	}

	file << "# Configuration file\n\n";
	for (const auto& default_section : configSections) 
	{
		file << "[" << default_section.name << "]\n";
		for (const auto& item : default_section.items) 
		{
			std::string currentValue = sections.at(default_section.name).getValues().at(item.name)->toString();
			file << item.name << " = " << currentValue
				 << " # type: " << item.type
				 << ", description: " << item.description;
			
			if (item.validationRule) 
			{
				file << " (validationRule: " << item.validationRule->toString() << ")";
			}
			file << "\n";
		}
		file << "\n";
	}
	
	if(!instructions_footer.empty())
		file << instructions_footer;
	
	std::cout << "Finished saving the current configuration to: " << this->filepath<< std::endl;
}

} // namespace ConfigLib
