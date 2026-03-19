#include <stdexcept>
#include <iostream>
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

bool tryParseLibProvidedCLIFlags(
	const std::string& arg,
	LibProvidedCLIFlags& flags)
{
	if(arg == "--help") {flags.help = true; return true;}
	if(arg == "--print") {flags.print = true; return true;}
	if(arg == "--save") {flags.save = true; return true;}
	if(arg == "--reset") {flags.reset = true; return true;}
	if(arg == "--diff") {flags.diff = true; return true;}
	
	const std::string flatPrefix = "--flat=";
	const std::string configPrefix = "--config=";
	const std::string exportPrefix = "--export=";
	
	if(arg.substr(0,flatPrefix.size()) == flatPrefix)
	{
		const std::string value = arg.substr(flatPrefix.size());
		if(value.empty())
			throw std::runtime_error(
				"--flat= requires a boolean value. "
				"Examples: ---flat=true, --flat=false, --flat=yes, --flat=0");
		
		try{TypeParser<bool>::fromString(value);}
		catch(const std::exception& e)
		{
			throw std::runtime_error(
				std::string("Invalid value for --flat=: ") + e.what() + ". "
				"Examples: --flat=true, --flat=false, --flat=yes, --flat=0");
		}
		flags.flat = value;
		return true;
			
	}
	
	if(arg.substr(0,configPrefix.size()) == configPrefix)
	{
		flags.config_path = arg.substr(configPrefix.size());
		if(flags.config_path.empty())
			throw std::runtime_error(
				"--config= requires a file path. Example: --config=my_settings.ini");
		
		return true;
	}
	
	if(arg.substr(0,exportPrefix.size()) == exportPrefix)
	{
		flags.export_path = arg.substr(exportPrefix.size());
		if(flags.export_path.empty())
			throw std::runtime_error(
				"--export= requires a file path. Example: --export=output.ini");
		
		return true;
	}
	
	return false;
}

void parseIntoParsedCLIArgs(
	const std::string& arg, 
	const std::string& body,
	const SchemaLookup& schemaLookup,
	std::map<std::pair<std::string, std::string>, std::string>& values)
{
	// TODO (IHT 20260308): could be section.key= value, section.key =value, section.key = value or without the section...
	const auto dotPos = body.find('.');
	if(dotPos == std::string::npos)
		throw std::runtime_error(
			"Unrecognized argument '" + arg + "'. "
			"To set a config value use --section.key=value "
			"To see available flags, run with --help.");
			
	const auto eqPos = body.find('=', dotPos+1); 
	if(eqPos == std::string::npos)
		throw std::runtime_error(
			"Malformed argument '" + arg + "': missing '=' after key. "
			"Expected --"+body.substr(0,eqPos) + "=<value>.");
			
	const std::string section = body.substr(0, dotPos);
	const std::string key = body.substr(dotPos+1, eqPos-dotPos-1);
	const std::string value = body.substr(eqPos +1);
	
	if(section.empty())
		throw std::runtime_error(
            "Malformed argument '" + arg + "': section name is empty. "
            "Expected --section.key=value.");
    
    if(key.empty())
		throw std::runtime_error(
            "Malformed argument '" + arg + "': key name is empty. "
            "Expected --"+section+".key=value.");
            
	const auto sectionIt = schemaLookup.find(section);
	if(sectionIt ==schemaLookup.end())
		throw std::runtime_error(
            "Unknown section '" + section + "' in argument '"+arg+"'."
            "To see available sections and keys, run with  --help.");
            
	if(sectionIt->second.find(key) == sectionIt->second.end())
		throw std::runtime_error(
            "Unknown key '"+key+"' in section '" + section + "' in argument '"+arg+"'."
            "To see available sections and keys, run with --help.");
            
	values[{section,key}] = value;
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
		
		if(tryParseLibProvidedCLIFlags(arg, result.flags))
			continue;
		
		parseIntoParsedCLIArgs(arg, 
				arg.substr(2), 
				schemaLookup, 
				result.values);
	}
	
	return result;
}

} // namespace ConfigLib
