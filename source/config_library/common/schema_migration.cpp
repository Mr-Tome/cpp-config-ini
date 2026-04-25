#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <ostream>
#include "schema_migration.hpp"
#include "config_lib_internal_utility.hpp"

namespace ConfigLib
{

namespace Migration
{
		
uint32_t parseSchemaVersion(const std::string& filePath)
{	
	std::ifstream file(filePath);
	if(!file.is_open())
		return invalidSchemaVersion; // normal on first run ever
		
	const std::string schemaString = Migration::schemaVersionPrefix;
	std::string line;
	
	while(std::getline(file, line))
	{
		const std::string trimmed = Internal::trim(line);
		
		if (trimmed.empty())
			continue;
		
		
		if (trimmed.size() >= schemaString.size() && 
			trimmed.substr(0, schemaString.size()) == schemaString)
		{
			try
			{
				return static_cast<uint32_t>(std::stoul(trimmed.substr(schemaString.size())));
			}
			catch(...)
			{
				return Migration::invalidSchemaVersion; // TODO (IHT 2026.04.07): need to do a static_assert so developers dont start at 0.
			}
		}
		
		
		//requires the schemaString to be at the top of the file.
		if(trimmed[0] != '#')
			break;
	}
	
	return Migration::invalidSchemaVersion;
}

namespace //anonymous
{

//collection of migrations whose range covers the gap between 
//the file's version and the schema's version
//the return a vector of versions an ascending order.
std::vector<const SchemaMigration*> collectMigrationsAcrossVersionGap(
	const std::vector<SchemaMigration>& migrations,
	uint32_t fileVersion,
	uint32_t schemaVersion)
{
	std::vector<const SchemaMigration*> result;
	
	for (const auto& m : migrations)
		if(m.fromVersion>= fileVersion && m.toVersion <= schemaVersion)
			result.push_back(&m);
	
	std::sort(result.begin(), result.end(), 
			[](const SchemaMigration* a, const SchemaMigration* b)
			{
				return a->fromVersion < b->fromVersion;
			});
			
	return result;
}

std::string maybeTransform(
	const std::string& value,
	const std::function<std::string(const std::string&)>& fn)
{
	return fn ? fn(value) : value;
}

void applyRenames(
	RawConfigMap& rawConfig,
	const std::vector<const SchemaMigration*>& applicable,
	MigrationResult& result)
{
	for (const auto* m : applicable)
	{
		if (m->kind != SchemaMigration::Kind::Rename) 
			continue;
		if (!rawConfig.count(m->oldSection)) 
			continue;
		if (!rawConfig.at(m->oldSection).count(m->oldKey)) 
			continue;

		const std::string oldValue = rawConfig.at(m->oldSection).at(m->oldKey);
		const std::string newValue = maybeTransform(oldValue, m->transform);

		// if we erase first and it's a rename-to-self (transformInPlace):
		// will avoid a redundant copy
		rawConfig[m->oldSection].erase(m->oldKey);
		if (rawConfig.at(m->oldSection).empty())
			rawConfig.erase(m->oldSection);

		rawConfig[m->newSection][m->newKey] = newValue;
		
		MigrationChange change;
		change.oldSection = m->oldSection;
		change.oldKey     = m->oldKey;
		change.newSection = m->newSection;
		change.newKey     = m->newKey;
		change.oldValue   = oldValue;
		change.newValue   = newValue;
		result.changes.push_back(change);
	}
}

void applyRenameSections(
	RawConfigMap& rawConfig,
	const std::vector<const SchemaMigration*>& applicable,
	MigrationResult& result)
{
	for (const auto* m : applicable)
	{
		if (m->kind != SchemaMigration::Kind::RenameSection) 
			continue;
		if (!rawConfig.count(m->oldSection)) 
			continue;

		// a) RenameKey children...move from oldSection to newSection with optional transform
		for (const auto& child : m->children)
		{
			if (child.kind != SectionScopedMigration::Kind::RenameKey) 
				continue;
			if (!rawConfig.at(m->oldSection).count(child.oldKey)) 
				continue;

			const std::string oldValue = rawConfig.at(m->oldSection).at(child.oldKey);
			const std::string newValue = maybeTransform(oldValue, child.transform);
			rawConfig[m->newSection][child.newKey] = newValue;
			rawConfig[m->oldSection].erase(child.oldKey);
			
			MigrationChange change;
			change.oldSection = m->oldSection;
			change.oldKey     = child.oldKey;
			change.newSection = m->newSection;
			change.newKey     = child.newKey;
			change.oldValue   = oldValue;
			change.newValue   = newValue;
			result.changes.push_back(change);
		}

		// b) move all remaining keys from oldSection to newSection.
		for (const auto& kv : rawConfig.at(m->oldSection))
		{
			rawConfig[m->newSection][kv.first] = kv.second;
			
			MigrationChange change;
			change.oldSection = m->oldSection;
			change.oldKey     = kv.first;
			change.newSection = m->newSection;
			change.newKey     = kv.first;
			change.oldValue   = kv.second;
			change.newValue   = kv.second;
			result.changes.push_back(change);
		}
		rawConfig.erase(m->oldSection);

		// c) TransformKey children...all keys are now in newSection at their final names.
		for (const auto& child : m->children)
		{
			if (child.kind != SectionScopedMigration::Kind::TransformKey) 
				continue;
			if (!rawConfig.at(m->newSection).count(child.oldKey)) 
				continue;

			if (!child.transform)
				throw std::runtime_error(
					"Migration::transformKey for key '" + child.oldKey
					+ "' in renameSection '" + m->oldSection + "' -> '"
					+ m->newSection + "' has a null transform function.");

			const std::string oldValue = rawConfig.at(m->newSection).at(child.oldKey);
			const std::string newValue = child.transform(oldValue);
			rawConfig[m->newSection][child.oldKey] = newValue;

			// update the entire section move change record if it exists, otherwise add new entry
			bool found = false;
			for (auto& change : result.changes)
			{
				if (change.newSection == m->newSection && change.newKey == child.oldKey)
				{
					change.newValue = newValue;
					found = true;
					break;
				}
			}
			if (!found)
			{
				MigrationChange change;
				change.oldSection = m->newSection;
				change.oldKey     = child.oldKey;
				change.newSection = m->newSection;
				change.newKey     = child.oldKey;
				change.oldValue   = oldValue;
				change.newValue   = newValue;
				result.changes.push_back(change);
			}
		}
	}
}

} // namespace anonymous

std::pair<RawConfigMap, MigrationResult> applyMigrations(
	RawConfigMap rawConfig,
	const std::vector<SchemaMigration>& migrations,
	uint32_t fileVersion,
	uint32_t schemaVersion)
{
	MigrationResult result;
	result.fileVersion = fileVersion;
	result.schemaVersion = schemaVersion;
	
	const auto applicable = collectMigrationsAcrossVersionGap(migrations, 
											fileVersion, schemaVersion);

	applyRenames(rawConfig, applicable, result);
	applyRenameSections(rawConfig, applicable, result);
	
	return {std::move(rawConfig), std::move(result)};
}

void logMigrationResult(
	const MigrationResult& result,
	const std::string& filePath,
	std::ostream& out)
{
	if (!result.ranAny())
	{
		out << "[Migration] No migrations applied to '" << filePath << "'"
		    << " (file v" << result.fileVersion
		    << " already at schema v" << result.schemaVersion << ").\n"
		    << "[Migration] Note: NoConflict from schema evolver is expected"
		       " when migrations pre-transformed all values.\n";
		return;
	}

	out << "[Migration] " << result.changes.size()
	    << " migration(s) applied to '" << filePath << "'"
	    << " (file v" << result.fileVersion
	    << " -> schema v" << result.schemaVersion << "):\n";

	for (const auto& c : result.changes)
	{
		const bool keyMoved     = (c.oldSection != c.newSection || c.oldKey != c.newKey);
		const bool valueChanged = (c.oldValue != c.newValue);

		if (keyMoved && valueChanged)
			out << "[Migration]   Renamed+Transformed  "
			    << c.oldSection << "." << c.oldKey
			    << " -> " << c.newSection << "." << c.newKey
			    << "  '" << c.oldValue << "' -> '" << c.newValue << "'\n";
		else if (keyMoved)
			out << "[Migration]   Renamed  "
			    << c.oldSection << "." << c.oldKey
			    << " -> " << c.newSection << "." << c.newKey
			    << "  (value '" << c.oldValue << "' unchanged)\n";
		else if (valueChanged)
			out << "[Migration]   Transformed  "
			    << c.oldSection << "." << c.oldKey
			    << "  '" << c.oldValue << "' -> '" << c.newValue << "'\n";
		// identical key and value = idempotent migration, not worth logging
	}
}

} // namespace Migration
} // namespace ConfigLib
