#include <algorithm>
#include <fstream>
#include <stdexcept>
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
	const std::vector<const SchemaMigration*>& applicable)
{
	for (const auto* m : applicable)
	{
		if (m->kind != SchemaMigration::Kind::Rename) 
			continue;
		if (!rawConfig.count(m->oldSection)) 
			continue;
		if (!rawConfig.at(m->oldSection).count(m->oldKey)) 
			continue;

		const std::string value = maybeTransform(rawConfig.at(m->oldSection).at(m->oldKey), m->transform);

		// if we erase first and it's a rename-to-self (transformInPlace):
		// will avoid a redundant copy
		rawConfig[m->oldSection].erase(m->oldKey);
		if (rawConfig.at(m->oldSection).empty())
			rawConfig.erase(m->oldSection);

		rawConfig[m->newSection][m->newKey] = value;
	}
}

void applyRenameSections(
	RawConfigMap& rawConfig,
	const std::vector<const SchemaMigration*>& applicable)
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

			rawConfig[m->newSection][child.newKey] = maybeTransform(rawConfig.at(m->oldSection).at(child.oldKey), child.transform);
			rawConfig[m->oldSection].erase(child.oldKey);
		}

		// b) move all remaining keys from oldSection to newSection.
		for (const auto& kv : rawConfig.at(m->oldSection))
			rawConfig[m->newSection][kv.first] = kv.second;
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

			rawConfig[m->newSection][child.oldKey] = child.transform(rawConfig.at(m->newSection).at(child.oldKey));
		}
	}
}

} // namespace anonymous

RawConfigMap applyMigrations(
	RawConfigMap rawConfig,
	const std::vector<SchemaMigration>& migrations,
	uint32_t fileVersion,
	uint32_t schemaVersion)
{
	auto applicable = collectMigrationsAcrossVersionGap(migrations, 
											fileVersion, schemaVersion);

	applyRenames(rawConfig, applicable);
	applyRenameSections(rawConfig, applicable);
	
	return rawConfig;
	
}

} // namespace Migration
} // namespace ConfigLib
