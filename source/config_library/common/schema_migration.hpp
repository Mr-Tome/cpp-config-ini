#pragma once
#include <functional>
#include <string>

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
	int fromVersion;
	int toVersion;
	
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
	
inline SchemaMigration rename(
	int fromVersion,
	int toVersion,
	const std::string& oldSection, const std::string& oldKey
	const std::string& newSection, const std::String& newKey)
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
	int fromVersion,
	int toVersion,
	const std::string& oldSection, const std::string& oldKey,
	std::function<std::string(const std::string&)> fn)
{
	SchemaMigration m;
	m.kind = SchemaMigration::Kind::Transform;
	m.fromVersion = fromVersion;
	m.toVersion = toVersion;
	m.oldSection = oldSection;
	m.oldKey = oldKey;
	m.transform = std::move(fn);
	return m;
}

} // namespace Migration

} // namespace ConfigLib
