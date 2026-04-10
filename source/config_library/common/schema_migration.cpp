#include <algorithm>
#include <fstream>
#include <stdexcept>
#include "schema_migration.hpp"
#include "config_lib_internal_utility.hpp"

namespace ConfigLib
{

namespace Migration
{
		
// scan 'filePath' for a leading comment. If the line is 
// of the form: # __schema_version__ = N,
// then:
// Returns N if found, 0 otherwise. Stops scanning at the first non-comment line.	
uint32_t parseSchemaVersion(const std::string& filePath)
{	
	std::ifstream file(filePath);
	if(!file.is_open())
		return 0; // should throw?
		
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
	std::vector<const SchemaMigration*> applicable;
	
	for (const auto& m : migrations)
		if(m.fromVersion>= fileVersion && m.toVersion <= schemaVersion)
			applicable.push_back(&m);
	
	std::sort(applicable.begin(), applicable.end(), 
			[](const SchemaMigration* a, const SchemaMigration* b)
			{
				return a->fromVersion < b->fromVersion;
			});
			
	return applicable;
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

	for(const auto* m: applicable)
	{
		if(m->kind == SchemaMigration::Kind::Rename)
		{
			//old section doesnt exist in current config ini
			if(!rawConfig.count(m->oldSection))
				continue;
			
			// old section still exists and so does old key
			if(rawConfig.at(m->oldSection).count(m->oldKey))
			{
				//take the old value 
				rawConfig[m->newSection][m->newKey] = rawConfig.at(m->oldSection).at(m->oldKey);
			
				//erase old value.
				rawConfig[m->oldSection].erase(m->oldKey);
			}
			
			//if all the old keys are gone, we can erase the old section.
			if (rawConfig.at(m->oldSection).empty())
				rawConfig.erase(m->oldSection);
			
		}
		if(m->kind ==  SchemaMigration::Kind::Transform)
		{
			if(!rawConfig.count(m->oldSection))//old section doesnt exist in current config ini
				continue;
			
			// old section still exists, but old key does not
			if(!rawConfig.at(m->oldSection).count(m->oldKey)) 
				continue;
				
			if (!m->transform)
				throw std::runtime_error(
					"SchemaMigration::Kind::Transform for "
					+ m->oldSection + "." + m->oldKey
					+ " has a null transform function.");
			
			rawConfig[m->oldSection][m->oldKey] = m->transform(rawConfig.at(m->oldSection).at(m->oldKey));
				
		}
		else
			throw std::logic_error("You forgot to update "
				"sceham_migrations.cpp::applyMigrations switch cases");
	}
	
	return rawConfig;
	
}

} // namespace Migration
} // namespace ConfigLib
