#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include "ini_config_reader.hpp"
#include "../common/config_lib_internal_utility.hpp"
#include "../common/type_parser.hpp"
#include "../common/schema_evolver.hpp"
#include "../common/config_value.hpp"

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


bool parseRawINI(
    const std::string& filePath,
    std::map<std::string, std::map<std::string, std::string>>& rawConfigOut)
{
	std::ifstream file(filePath);
	if (!file.is_open()) 
	{
		std::cerr << "Unable to open file: " << filePath << std::endl;
		return false;
	}
	
	std::string current_section;
	std::string line;
	
	while (std::getline(file, line)) 
	{
		line = ConfigLib::Internal::trim(line);
		if (line.empty() || line[0] == '#') 
			continue;

		if (line[0] == '[' && line.back() == ']') 
		{
			current_section = line.substr(1, line.size() - 2);
			continue;
		} 
		
		const auto eqPos = line.find('=');
		
		if(eqPos == std::string::npos) 
			continue;
		
		if(current_section.empty()) 
			continue;
		
		std::string key = ConfigLib::Internal::trim(line.substr(0, eqPos));
		std::string value = line.substr(eqPos + 1);
		
		// remove the comments from the value
		const size_t commentPos = value.find('#'); //TODO: if a user puts a '#' in a std::string uh oh...
		if (commentPos != std::string::npos) 
			value = value.substr(0, commentPos);
			
		value = ConfigLib::Internal::trim(value);
		
		rawConfigOut[current_section][key] = value;
	}
	return true;
}
	
void loadConfigFromFile(
	const std::string& filePath,
	const std::vector<ConfigSection>& configSections,
	std::unordered_map<std::string, ConfigSectionStore>& sections) 
{
	std::cout << "Calling loadConfigFromFile()" << std::endl;
		
	RawConfigMap rawINI;
	if (!parseRawINI(filePath, rawINI)) 
	{
		std::cerr << "Unable to open file: " << filePath << std::endl;
		return;
	}

	const auto schemaLookup = buildSchemaItemLookup(configSections);
			
	auto& registry = TypeRegistry::instance();
	
	for(const auto& rawSection : rawINI)
	{
		const auto sectionIt = schemaLookup.find(rawSection.first);
		if (sectionIt == schemaLookup.end()) 
			continue;
			
		for (const auto& rawItem : rawSection.second)
		{
			const auto itemIt = sectionIt->second.find(rawItem.first);
			if (itemIt == sectionIt->second.end()) 
				continue;
				
			const ConfigItem& item = *itemIt->second;
			const auto& value = rawItem.second;
			try 
			{
				auto parsedValue = registry.parseValue(item.type, value);
				
				//if there's a rule, let's validate against it.
				if (!item.validationRule || (*item.validationRule)(*parsedValue)) 
				{
					sections[rawSection.first].getValues()[rawItem.first] = parsedValue;
				} 
				else 
				{
					std::cerr << "Validation failed for " 
							  << rawSection.first << "." << rawItem.first 
							  << ". Using default value." << std::endl;
					Internal::applyDefaultValue(rawSection.first, rawItem.first, item, sections);
				}
			} 
			catch (const std::exception& e) 
			{
				std::cerr << "Error processing " 
						  << rawSection.first << "." << rawItem.first
						  << ": " << e.what() << ". Using default value." << std::endl;
				Internal::applyDefaultValue(rawSection.first, rawItem.first, item, sections);
			}	
		}
	}
				
	std::cout << "Config loaded" << std::endl;
}

std::string formatINI(
	const std::vector<ConfigSection>& configSections,
	std::function<std::pair<std::string, std::string>(
		const ConfigSection&, const ConfigItem&)> valueSource,
	std::function<std::string(const std::string& sectionName)> postSectionLines,
	const std::string& trailingContent,
	const std::string& header)
{
	//std::cout << "Calling formatINI(...)" << std::endl;
	
	//version 
	std::string output;
	if (!header.empty())
		output = header + "\n\n";
	
	output += "# Configuration file\n\n";
	for (const auto& section : configSections)
	{
		output += "[" + section.name + "]\n";

		for (const auto& item : section.items)
		{
			if (item.persistence == Persistence::Volatile)
			{
				output += "# " + item.name + " = <not stored>"
					+ " # type: "        + item.type
					+ ", description: "  + item.description
					+ " [VOLATILE: supply via CLI every run]\n";
				continue;
			}

			const auto vSource = valueSource(section, item);
			const auto& value = vSource.first;
			const auto& evolutionNote = vSource.second;

			output += item.name + " = " + value
				+ " # type: "       + item.type
				+ ", description: " + item.description;

			if (item.validationRule)
				output += " (validationRule: " + item.validationRule->toString() + ")";

			output += evolutionNote;
			output += "\n";
		}

		output += postSectionLines(section.name);
		output += "\n";
	}

	output += trailingContent;
	output += iniInstructions();
	
	//std::cout << "Completed formatINI(...)" << std::endl;
	return output;
}

std::string evolveINI(
    const std::vector<ConfigSection>& currentSchema,
    const RawConfigMap& rawConfig,
    const SchemaEvolutionResult& result,
    OrphanedConfigItemPolicy policy,
    const std::string& header)
{
	//TODO(IHT: 2026.04.02) consolidate lookups with schema_evolver.cpp
	std::unordered_set<std::string> addedKeySet;
	for (const auto& addedSection : result.addedSections)
		for (const auto& item : addedSection.items)
			addedKeySet.insert(addedSection.name + "." + item.name);
    
	std::unordered_map<std::string, const ConfigItemConflict*> conflictLookup;
	for (const auto& sc : result.conflictingSections)
		for (const auto& conflict : sc.conflictingConfigItems)
			conflictLookup[sc.sectionName + "." + conflict.configItem.name] = &conflict;
			
	std::unordered_set<std::string> schemaKeySet;
	std::unordered_set<std::string> schemaSectionSet;
	for (const auto& section : currentSchema)
	{
		schemaSectionSet.insert(section.name);
		for (const auto& item : section.items)
			schemaKeySet.insert(section.name + "." + item.name);
	}
	
	//returns the {value, evaluation comment} per config item.
	auto newValueComment = [&](const ConfigSection& section, const ConfigItem& item)
        -> std::pair<std::string, std::string>
    {		
		const std::string qualifiedKey = section.name + "." + item.name;
		if (addedKeySet.count(qualifiedKey))
			return {item.defaultValue, " # (added by schema update)"};
			
		const auto conflictIt = conflictLookup.find(qualifiedKey);
		if (conflictIt != conflictLookup.end())
		{
			const ConfigItemConflict& conflict = *conflictIt->second;

			if (conflict.conflictType == ConfigItemConflictType::NoConflict)
				return {conflict.newFileValue, ""};

			const std::string ruleStr = item.validationRule
				? item.validationRule->toString() : "unknown";

			const std::string note = (conflict.conflictType == ConfigItemConflictType::TypeMismatch)
				? " # (schema update: previous value '" + conflict.previousFileValue
					+ "' is incompatible with new type '" + item.type + "', reset to default)"
				: " # (schema update: previous value '" + conflict.previousFileValue
					+ "' failed validation rule '" + ruleStr + "', reset to default)";

			return {conflict.newFileValue, note};
		}

		//no conflict
		throw std::logic_error(
			"ini_config_reader.cpp, evolveINI: no entry in conflictLookup for " + qualifiedKey
			+ "! This is a bug in evolveFileWithSchema!");
	};
	
	auto deprecationKeyComments = [&](const std::string& sectionName) -> std::string
	{
		if (policy != OrphanedConfigItemPolicy::CommentOut)
			return "";

		const auto rawSectionIt = rawConfig.find(sectionName);
		if (rawSectionIt == rawConfig.end())
			return "";

		std::string lines;
		for (const auto& rawItem : rawSectionIt->second)
		{
			if (!schemaKeySet.count(sectionName + "." + rawItem.first))
				lines += "# [deprecated] " + rawItem.first
					   + " = " + rawItem.second + "\n";
		}
		return lines;
	};
	
	
	std::string orphanedSections;
    for (const auto& rawSection : rawConfig)
    {
        if (schemaSectionSet.count(rawSection.first))
            continue;

        if (policy == OrphanedConfigItemPolicy::Remove)
            continue;

        // CommentOut (RuntimeError already threw in evolveFileWithSchema):
        orphanedSections += "# [deprecated section: " + rawSection.first + "]\n";
        for (const auto& rawItem : rawSection.second)
            orphanedSections += "# [deprecated] " + rawItem.first
                             + " = " + rawItem.second + "\n";
        orphanedSections += "\n";
    }

    return formatINI(currentSchema, newValueComment, deprecationKeyComments, orphanedSections, header);    
}

} // namespace ConfigLib
