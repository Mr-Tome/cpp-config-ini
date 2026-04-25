#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "schema_evolver.hpp"
#include "config_value.hpp"
#include "type_parser.hpp"


namespace ConfigLib 
{

namespace
{

std::unordered_set<std::string> buildSchemaKeySet(
    const std::vector<ConfigSection>& schema)
{
    std::unordered_set<std::string> keys;
    for (const auto& section : schema)
        for (const auto& item : section.items)
            keys.insert(section.name + "." + item.name);
    return keys;
}	

ConfigItemConflictType classifyConflict(
    const ConfigItem& schemaItem,
    const std::string& fileRawValue,
    std::string& adoptedValue)
{
	auto& registry = TypeRegistry::instance();
	
	std::shared_ptr<ConfigValue> parsed;
	//there's a priority here... 
	//TypeMismatch > ValidationFailure > NoConflict
	try
    {
        parsed = registry.parseValue(schemaItem.type, fileRawValue);
    }
    catch (const std::exception&)
    {
        adoptedValue = schemaItem.defaultValue;
        return ConfigItemConflictType::TypeMismatch;
    }
    
    if (schemaItem.validationRule && !(*schemaItem.validationRule)(*parsed))
    {
        adoptedValue = schemaItem.defaultValue;
        return ConfigItemConflictType::ValidationFailure;
    }
    
    adoptedValue = fileRawValue;
    return ConfigItemConflictType::NoConflict;
}

SectionConflict& findOrInsertSectionConflict(
    SchemaEvolutionResult& result,
    const std::string& sectionName)
{
    for (auto& conflictingSection : result.conflictingSections)
        if (conflictingSection.sectionName == sectionName)
            return conflictingSection;

    SectionConflict newConflictingSection;
    newConflictingSection.sectionName = sectionName;
    result.conflictingSections.push_back(newConflictingSection);
    return result.conflictingSections.back();
}

} // anonymous namespace
	
SchemaEvolutionResult evolveFileWithSchema(
	const RawConfigMap& rawConfig,
	const std::vector<ConfigSection>& currentSchema,
	OrphanedConfigItemPolicy policy)
{
	SchemaEvolutionResult result;
	
	const auto schemaLookup = buildSchemaLookup(currentSchema);
	const auto schemaKeySet = buildSchemaKeySet(currentSchema);
	
	for (const auto& section : currentSchema)
    {
        ConfigSection addedInThisSection;
        addedInThisSection.name = section.name;
        
        for (const auto& item : section.items)
        {
			 const bool keyInFile = rawConfig.count(section.name) &&
                rawConfig.at(section.name).count(item.name);
                
			if (item.persistence == Persistence::Volatile)
			{
				if (keyInFile)
					result.fileModified = true;
				continue;
			}           
            
            if(!keyInFile)
            {
				addedInThisSection.items.push_back(item);
				result.fileModified = true;
			}
			else
			{
				
				const std::string& fileRawValue = rawConfig.at(section.name).at(item.name);

                std::string adoptedValue;
                const auto conflictType = classifyConflict(item, fileRawValue, adoptedValue);

						
				if (conflictType != ConfigItemConflictType::NoConflict)
                	result.fileModified = true;
				
                const ConfigItemConflict configItemConflict {item, 
						conflictType, fileRawValue, adoptedValue};
					findOrInsertSectionConflict(result, section.name).conflictingConfigItems.push_back(configItemConflict);

			}
		}
		
		if (!addedInThisSection.items.empty())
            result.addedSections.push_back(addedInThisSection);
	}
	
	//now we are looking to see if there's any orphaned keys.
	for (const auto& rawSection : rawConfig)
    {
        for (const auto& rawItem : rawSection.second)
        {
            const std::string qualifiedKey = rawSection.first + "." + rawItem.first;
            if (schemaKeySet.count(qualifiedKey))
                continue;

            if (policy == OrphanedConfigItemPolicy::RuntimeError)
                throw std::runtime_error(
                    "Schema evolution: key '" + qualifiedKey + "' exists in "
                    "the config file but not in the current schema. "
                    "Remove to suppress this error.");

            OrphanedConfigItem orphan;
            orphan.sectionName = rawSection.first;
            orphan.configItemName = rawItem.first;
            orphan.rawValue = rawItem.second;

            findOrInsertSectionConflict(result, rawSection.first).removedEntries.push_back(orphan);

            result.fileModified = true;
        }
    }

    return result;
}

void logEvolutionResult(
	const SchemaEvolutionResult& result,
	const std::string& filePath,
	std::ostream& out)
{
	const std::size_t n = result.numberOfConflicts();
	out << "[SchemaEvolver] " << SchemaEvolutionResult::to_string(result) << "\n";
	out << "[SchemaEvolver] " << n << " change(s) applied to '" << filePath << "'.\n";

	for (const auto& addedSection : result.addedSections)
	{
		for (const auto& item : addedSection.items)
		{
			out << "[SchemaEvolver]   Added    "
			    << addedSection.name << "." << item.name
			    << "  (type: " << item.type
			    << ", default: " << item.defaultValue << ")\n";
		}
	}

	for (const auto& sc : result.conflictingSections)
	{
		for (const auto& conflict : sc.conflictingConfigItems)
		{
			if (conflict.conflictType == ConfigItemConflictType::NoConflict)
				continue;

			const std::string reason =
				(conflict.conflictType == ConfigItemConflictType::TypeMismatch)
				? "TypeMismatch: incompatible with type '" + conflict.configItem.type + "'"
				: "ValidationFailure: " + (conflict.configItem.validationRule
					? conflict.configItem.validationRule->toString()
					: "unknown rule");

			out << "[SchemaEvolver]   Reset    "
			    << sc.sectionName << "." << conflict.configItem.name
			    << "  old='" << conflict.previousFileValue
			    << "' -> default='" << conflict.newFileValue
			    << "'  (" << reason << ")\n";
		}

		for (const auto& orphan : sc.removedEntries)
		{
			out << "[SchemaEvolver]   Orphan   "
			    << orphan.sectionName << "." << orphan.configItemName
			    << " = " << orphan.rawValue
			    << "  (not in schema, see [deprecated] comment in file)\n";
		}
	}
}

} // namespace ConfigLib
