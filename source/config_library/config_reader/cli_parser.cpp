#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include "cli_parser.hpp"

namespace ConfigLib
{

namespace // anonymous
{

using SchemaLookup = std::unordered_map<std::string, std::unordered_set<std::string>>;

SchemaLookup buildSchemaLookup(const std::vector<ConfigSection>& sections)
{
	SchemaLookup lookup;
	
	for (const auto& section : sections)
		for (const auto& item : section.items)
			lookup[section.name].insert(item.name);
			
	return lookup;
}

} // namespace anonymous
	
	
ParsedCLIArgs parseCLIArgs(
	const std::vector<std::string>& rawArgs,
	const std::vector<ConfigSection>& sections)
{
	ParsedCLIArgs result;
	const SchemaLookup schemaLookup = buildSchemaLookup(sections);
	
	for (size_t i = 1; i < rawArgs.size(); ++i)
	{
		const std::string& arg = rawArgs[i];
		
		if(arg.size() < 2 || arg[0]!= '-' || arg[1]!= '-')
		{
			throw std::runtime_error(
				"Unrecognized argument '" + arg + "'."
				"All arguments must start with '--'."
				"Use --Section.key=value to override a config value."
				"or run --help to see all available options.");
		} 
		
		
	}
	
	return result;
}
} // namespace ConfigLib
