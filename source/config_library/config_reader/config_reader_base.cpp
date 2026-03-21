#include <fstream>
#include <sstream>
#include <typeinfo>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "config_reader_base.hpp"

namespace ConfigLib 
{
									
	
void ConfigReaderBase::initialize(
			const std::vector<ConfigSection>& configSections) 
{
	std::cout << "ConfigReader::initialize started" << std::endl;
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
		}

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
	
void ConfigReaderBase::validateForCLI(
			const std::vector<ConfigSection>& configSections) 
{
	std::cout << "ConfigReader::validateForCLI started" << std::endl;
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
		auto error_string = std::string("Exception in ConfigReader::validateForCLI: ") + e.what();
		throw std::runtime_error(error_string);
	} 
	catch (...) 
	{
		auto error_string = std::string("Unknown exception in ConfigReader::validateForCLI");
		throw std::runtime_error(error_string);
	}
	std::cout << "ConfigReader::validateForCLI finished" << std::endl;
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

namespace Internal
{
	
void applyDefaultValue(
	const std::string& sectionName,
	const std::string& key,
	const ConfigItem& item,
	std::unordered_map<std::string, ConfigSectionStore>& sections)
{
	auto& registry = TypeRegistry::instance();
	try 
	{
		auto parsedValue = registry.parseValue(item.type, item.defaultValue);
		sections[sectionName].getValues()[key] = parsedValue;
	} 
	catch (const std::exception& e) 
	{
		std::cerr << "ERROR: Default value '" << item.defaultValue << "' for " 
				  << sectionName << "." << key << " failed to parse: " << e.what() << std::endl;
		throw std::runtime_error("Invalid default value in schema for " + sectionName + "." + key);
	}
}

} //namespace ConfigLib::Internal
  	
void ConfigReaderBase::useDefaultValue(const std::string& section,
								   const std::string& key, 
								   const ConfigItem& item) 
{
	Internal::applyDefaultValue(section, key, item, this->sections);
}
	
void ConfigReaderBase::saveConfig(
		const std::vector<ConfigSection>& configSections,
		const std::string& path,
		const std::string& instructions_footer) const 
{
	std::cout << "Saving the current configuration to: " <<path<< std::endl;
	std::ofstream file(path);
	if (!file.is_open()) 
	{
		throw std::runtime_error("Unable to open file for writing: " + path);
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
			if(item.persistence == Persistence::Volatile)
			{
				file << " [VOLATILE: must be supplied via CLI every run.]";
			}
			file << "\n";
		}
		file << "\n";
	}
	
	if(!instructions_footer.empty())
		file << instructions_footer;
	
	std::cout << "Finished saving the current configuration to: " << path<< std::endl;
}

} // namespace ConfigLib
