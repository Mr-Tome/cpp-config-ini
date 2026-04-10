#pragma once
#include <functional>
#include <string>
#include <vector>
#include "schema_evolver.hpp"//just for RawConfigMap

namespace ConfigLib
{

struct SchemaMigration
{

	enum class Kind
	{
		Rename, // should transfer the users value from an old key name to a new one
		Transform //applies function to a key's raw string value
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
	
	//when it's a Kind::Rename, should be nullptr
	//when it's Kind::Transform
	std::function<std::string(const std::string&)> transform;
};	

namespace Migration
{

const static uint32_t invalidSchemaVersion = 0;

constexpr const char* schemaVersionPrefix = "# __schema_version__ = ";

inline SchemaMigration rename(
	uint32_t fromVersion,
	uint32_t toVersion,
	const std::string& oldSection, const std::string& oldKey,
	const std::string& newSection, const std::string& newKey)
{
	SchemaMigration m;
	m.kind = SchemaMigration::Kind::Rename;
	m.fromVersion = fromVersion;
	m.toVersion = toVersion;
	m.oldSection = oldSection;
	m.oldKey = oldKey;
	m.newSection = newSection;
	m.newKey = newKey;
	return m;
}

inline SchemaMigration transform(
	uint32_t fromVersion,
	uint32_t toVersion,
	const std::string& oldSection, const std::string& oldKey,
	std::function<std::string(const std::string&)> fn)
{
	if (!fn)
		throw std::invalid_argument(
			"Migration::transform for " + oldSection + "." + oldKey
			+ " requires a non-null transform function.");
			
	SchemaMigration m;
	m.kind = SchemaMigration::Kind::Transform;
	m.fromVersion = fromVersion;
	m.toVersion = toVersion;
	m.oldSection = oldSection;
	m.oldKey = oldKey;
	m.transform = std::move(fn);
	return m;
}

uint32_t parseSchemaVersion(const std::string& filePath);

RawConfigMap applyMigrations(
	RawConfigMap rawConfig,
	const std::vector<SchemaMigration>& migrations,
	uint32_t fileVersion,
	uint32_t schemaVersion);

} // namespace Migration

} // namespace ConfigLib
