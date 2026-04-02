#pragma once
#include <map>
#include <string>
#include <vector>
#include "config_schema.hpp"

namespace ConfigLib 
{

// when we find a section.configItem that no longer exists, i.e., an orphaned ConfigItem, 
// could be from a schema evolution (or a user fat-finger)
// this enum defines what should be done in the persistence layer.
enum class OrphanedConfigItemPolicy
{
	CommentOut, // should be default. could write # [deprecated] ConfigItem = value
	Remove, // silently remove from file
	RuntimeError // throws std::runtime_error
};

struct OrphanedConfigItem
{
	std::string sectionName;
	std::string configItemName;
	std::string rawValue;
};

// when we find a section.configItem that is conflicting with the current
// schema's section.configItem, this enum defines why. 
// (I think this could be a set)
enum class ConfigItemConflictType 
{
	NoConflict,
	//CommentUpdated, // kind of a catch all, but one of these must have changed: # type: string, description: attempt to cause bug [VOLATILE: supply via CLI every run]
	TypeMismatch, // the TypeRegistry's parseValue(newType, file's value), threw. (could be user error or could be schema change).
	ValidationFailure, // no TypeMismatch, but failed validation (could be user error or could be schema change).
	//BecameVolatile, // a value that once part of the file became volatile. Don't want to follow the OrphanedConfigItemPolicy, but it's tempting. i think the current impl already handles this nicely., e.g., # flat = <not stored> # type: string, description: attempt to cause bug [VOLATILE: supply via CLI every run] -- only relevant when a type pack has Config Reader's CRTP has CLI in it...
	//DefaultValueChanged // if a user never changed the default value, but the schema did, then should we change it?
};

struct ConfigItemConflict
{
	ConfigItem configItem;
	ConfigItemConflictType conflictType;
	std::string previousFileValue;
	std::string newFileValue;
};

struct SectionConflict
{
	std::string sectionName;
	std::vector<ConfigItemConflict> conflictingConfigItems;
	std::vector<OrphanedConfigItem> removedEntries;
};

struct SchemaEvolutionResult
{
	std::vector<ConfigSection> addedSections;
	std::vector<SectionConflict> conflictingSections;
	
	bool fileModified = false;
	
	std::size_t numberOfConflicts() const
	{
		std::size_t n = 0;
		for(const auto& section : addedSections)
			n += section.items.size();
		for(const auto& conflictingSection : conflictingSections)
		{
			n += conflictingSection.removedEntries.size();
			for(const auto& conflictingItem : conflictingSection.conflictingConfigItems)
				if(conflictingItem.conflictType != ConfigItemConflictType::NoConflict)
					++n;
		}
		return n;
	}
	
	bool isClean() const {return !fileModified;}
};

using RawConfigMap = std::map<std::string, std::map<std::string, std::string>>;

SchemaEvolutionResult evolveFileWithSchema(
	const RawConfigMap& rawConfig,
	const std::vector<ConfigSection>& currentSchema,
	OrphanedConfigItemPolicy policy = OrphanedConfigItemPolicy::CommentOut
);
	
} // namespace ConfigLib
