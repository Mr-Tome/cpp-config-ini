#pragma once
#include <functional>
#include <string>
#include <vector>
#include "schema_evolver.hpp"//just for RawConfigMap

namespace ConfigLib
{

struct SectionScopedMigration
{
	enum class Kind
	{
		RenameKey, // should transfer the users value from an old key name to a new one
		TransformKey // applies transform to key in newSection after all renames
	};
	
	Kind kind;
	std::string oldKey;
	std::string newKey; // unused for TransformKey
	std::function<std::string(const std::string&)> transform;
};


struct SchemaMigration
{

	enum class Kind
	{
		Rename, // should transfer the users value from an old key name to a new one
		RenameSection // moves all keys from old section to new section
		//Add, // i think these are handled via evolveFileWithSchema 
		//Remove // i think these are handled via evolveFileWithSchema 
	};
	
	Kind kind;
	uint32_t fromVersion;
	uint32_t toVersion;
	
	std::string oldSection;
	std::string oldKey;
	std::string newSection;
	std::string newKey;
	
	//optionally used for Kind::Rename. 
	// Transofrm should be utilized on pre-migration names, i.e., 
	//use transform on oldKeys, not newKeys.
	std::function<std::string(const std::string&)> transform;
	
	// Non-empty only for Kind::RenameSection.
	std::vector<SectionScopedMigration> children;
};	

namespace Migration
{

const static uint32_t invalidSchemaVersion = 0;

constexpr const char* schemaVersionPrefix = "# __schema_version__ = ";

// transfers the user's raw value from [oldSection].oldKey to [newSection].newKey.
// both oldKey and newKey are the pre-migration names
inline SchemaMigration rename(
	uint32_t fromVersion,
	uint32_t toVersion,
	const std::string& oldSection, const std::string& oldKey,
	const std::string& newSection, const std::string& newKey,
	std::function<std::string(const std::string&)> fn = nullptr) // optional transformer
{
	SchemaMigration m;
	m.kind = SchemaMigration::Kind::Rename;
	m.fromVersion = fromVersion;
	m.toVersion = toVersion;
	m.oldSection = oldSection;
	m.oldKey = oldKey;
	m.newSection = newSection;
	m.newKey = newKey;
	m.transform = std::move(fn);
	return m;
}

inline SectionScopedMigration renameKey(
	const std::string& oldKey,
	const std::string& newKey,
	std::function<std::string(const std::string&)> fn = nullptr)
{
	SectionScopedMigration c;
	c.kind = SectionScopedMigration::Kind::RenameKey;
	c.oldKey = oldKey;
	c.newKey = newKey;
	c.transform = std::move(fn);
	return c;
}
/* moves all keys from [oldSection] to [newSection]
 * Optional children have Migrations in the following order:
 * 1) RenameKey children
 * 2) move remaining keys from oldSection to newSection
 * 3) TransformKey children. (Key named by its final name in newSection)
 * 
 * Example:
 * Migration::renameSection(1, 2, "Physics", "Dynamics", [Physics] to [Dynamics]
 * {
 * 		Migration::renameKey("dt", "timestep"), Physics.dt to Dynamics.timestep
 * 		Migration::transformKey("timestep", scaleFn), scaleFn(Dynamics.timestep)
 * 		Migration::renameKey("mass", "mass", unitConvFn) renames to self, then unitConvFn(Dynamics.mass)
 * })
 * */
inline SchemaMigration renameSection(
	uint32_t fromVersion,
	uint32_t toVersion,
	const std::string& oldSection,
	const std::string& newSection,
	std::vector<SectionScopedMigration> children = {})
{
	SchemaMigration m;
	m.kind = SchemaMigration::Kind::RenameSection;
	m.fromVersion = fromVersion;
	m.toVersion = toVersion;
	m.oldSection = oldSection;
	m.newSection = newSection;
	m.children = std::move(children);
	return m;
}

inline SchemaMigration transformInPlace(
	uint32_t fromVersion, 
	uint32_t toVersion,
	const std::string& section, 
	const std::string& key,
	std::function<std::string(const std::string&)> fn)
{
	if (!fn)
		throw std::invalid_argument(
			"Migration::transformInPlace for " + section + "." + key
			+ " requires a non-null transform function.");
	
	return rename(fromVersion, toVersion, section, key, section, key, std::move(fn));
}

inline SectionScopedMigration transformKey(
	const std::string& key,
	std::function<std::string(const std::string&)> fn)
{
	if (!fn)
		throw std::invalid_argument(
			"Migration::transformKey for key '" + key
			+ "' requires a non-null transform function.");
	SectionScopedMigration c;
	c.kind = SectionScopedMigration::Kind::TransformKey;
	c.oldKey = key;
	c.transform = std::move(fn);
	return c;
}


//TODO(IHT: 2026.04.11): Consider moving this to anonymous namespace.
// scan 'filePath' for a leading comment. If the line is 
// of the form: # __schema_version__ = N, (which only works for INI atm...)
// then:
// returns N if found, invalidSchemaVersion (0) otherwise. 
// stops scanning at the first non-comment line and non-blank line.	
uint32_t parseSchemaVersion(const std::string& filePath);

// migrates in two passes.
// first pass is all RenameKey's (including transformInPlace)
// second pass is RenameSection
// Note: within each pass, migrations are applied in ascending fromVersion order, e.g.,
// 1,3,5,6,7,...,n 
RawConfigMap applyMigrations(
	RawConfigMap rawConfig,
	const std::vector<SchemaMigration>& migrations,
	uint32_t fileVersion,
	uint32_t schemaVersion);

} // namespace Migration

} // namespace ConfigLib
