#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include "cli_parser.hpp"
#include "../common/type_parser.hpp"

namespace ConfigLib
{
namespace Internal
{

namespace
{

bool tryParseLibProvidedCLIFlags(
	const std::string& arg,
	LibProvidedCLIFlags& flags)
{
	if(arg == "--help") {flags.help = true; return true;}
	if(arg == "--print") {flags.print = true; return true;}
	if(arg == "--save") {flags.save = true; return true;}
	if(arg == "--delete") {flags._delete = true; return true;}
	if(arg == "--diff") {flags.diff = true; return true;}

	if(arg == "--schema-dry-run") {flags.schema_dry_run = true; return true;}
	if(arg == "--schema-version") {flags.schema_version = true; return true;}

	const std::string flatPrefix   = "--flat=";
	const std::string configPrefix = "--config=";
	const std::string exportPrefix = "--export=";

	if(arg.starts_with(flatPrefix))
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

	if(arg.starts_with(configPrefix))
	{
		flags.config_path = arg.substr(configPrefix.size());
		if(flags.config_path.empty())
			throw std::runtime_error(
				"--config= requires a file path. Example: --config=my_settings.ini");
		return true;
	}

	if(arg.starts_with(exportPrefix))
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
	const CLIKeyMap& keyMap,
	bool flatEnabled,
	std::map<std::pair<std::string, std::string>, std::string>& values)
{
	const auto dotPos = body.find('.');

	if(dotPos == std::string::npos)
	{
		if(!flatEnabled)
			throw std::runtime_error(
				"Unrecognized argument '" + arg + "'. "
				"To set a config value use --section.key=value "
				"To see available flags, run with --help.");

		const auto eqPos = body.find('=');
		if(eqPos == std::string::npos)
			throw std::runtime_error(
				"Malformed argument '" + arg + "': missing '='. "
				"Expected --key=<value> or --Section.key=<value>.");

		const std::string flatKey = body.substr(0,eqPos);
		const std::string value   = body.substr(eqPos+1);

		if(flatKey.empty())
			throw std::runtime_error(
				"Malformed argument '" + arg + "': key name is empty. "
				"Expected --key=<value> or --Section.key=<value>.");

		if (keyMap.ambiguousKeysWhenFlat.contains(flatKey))
			throw std::runtime_error(
				"Ambiguous key '" + flatKey + "' in argument '" + arg + "': "
				"this key exists in multiple sections. "
				"Use the qualified form --Section." + flatKey + "=<value>. "
				"Run --help to see all sections that contain this key.");

		const auto it = keyMap.flatToQualified.find(flatKey);
		if (it == keyMap.flatToQualified.end())
			throw std::runtime_error(
				"Unknown key '" + flatKey + "' in argument '" + arg + "'. "
				"Run --help to see all available keys.");

		values[it->second] = value;
		return;
	}

	const auto eqPos = body.find('=', dotPos+1);
	if(eqPos == std::string::npos)
		throw std::runtime_error(
			"Malformed argument '" + arg + "': missing '=' after key. "
			"Expected --"+body.substr(0,dotPos) + "=<value>.");

	const std::string section = body.substr(0, dotPos);
	const std::string key     = body.substr(dotPos+1, eqPos-dotPos-1);
	const std::string value   = body.substr(eqPos+1);

	if(key.empty())
		throw std::runtime_error(
			"Malformed argument '" + arg + "': key name is empty. "
			"Expected --"+section+".key=<value>.");

	if(key.find('.') != std::string::npos)
		throw std::runtime_error(
			"Malformed argument '" + arg + "': key '"+key+"' contains a '.'."
			"Expected --" + section + ".key=<value>.");

	const auto sectionIt = schemaLookup.find(section);
	if(sectionIt == schemaLookup.end())
		throw std::runtime_error(
			"Unknown section '" + section + "' in argument '"+arg+"'."
			"To see available sections and keys, run with  --help.");

	if(!sectionIt->second.contains(key))
		throw std::runtime_error(
			"Unknown key '"+key+"' in section '" + section + "' in argument '"+arg+"'."
			"To see available sections and keys, run with --help.");

	values[{section,key}] = value;
}

void countAndFindOwnerOfConfigItems(
	const std::vector<ConfigSection>& sections,
	std::unordered_map<std::string, int>& keyCount,
	std::unordered_map<std::string, std::string>& keyOwner)
{
	for (const auto& section : sections)
	{
		for (const auto& item : section.items)
		{
			auto& configItemCount = ++keyCount[item.name];
			if(configItemCount == 1 || section.name.empty())
				keyOwner[item.name] = section.name;
		}
	}
}

const std::unordered_set<std::string>& reservedFlatKeys()
{
	static const std::unordered_set<std::string> keys = {
		"help", "print", "save", "delete", "diff",
		"flat", "config", "export",
		"schema-dry-run", "schema-version"
	};
	return keys;
}

} // anonymous namespace


CLIKeyMap buildCLIKeyMap(const std::vector<ConfigSection>& sections)
{
	CLIKeyMap result;

	std::unordered_map<std::string, int> keyCount;
	std::unordered_map<std::string, std::string> keyOwner;

	countAndFindOwnerOfConfigItems(sections, keyCount, keyOwner);

	for (const auto& section : sections)
	{
		for (const auto& item : section.items)
		{
			const auto pair      = std::make_pair(section.name, item.name);
			const int count      = keyCount.at(item.name);
			const bool ownerIsEmpty = keyOwner.at(item.name).empty();
			const bool unambiguous  = (count == 1) || ownerIsEmpty;

			if(unambiguous && keyOwner.at(item.name) == section.name)
			{
				result.flatToQualified[item.name] = pair;
				result.qualifiedToFlat[pair] = item.name;
			}
			else if ((count > 1 && !ownerIsEmpty) ||
					 reservedFlatKeys().contains(item.name))
			{
				result.ambiguousKeysWhenFlat.insert(item.name);
			}
		}
	}

	return result;
}

ParsedCLIArgs parseCLIArgs(
	const std::vector<std::string>& rawArgs,
	const std::vector<ConfigSection>& sections,
	const CLIKeyMap& keyMap,
	bool flatEnabled)
{
	ParsedCLIArgs result;
	const SchemaLookup schemaLookup = buildSchemaLookup(sections);

	for (size_t i = 1; i < rawArgs.size(); ++i)
	{
		const std::string& arg = rawArgs[i];

		if(arg.size() < 2 || arg[0] != '-' || arg[1] != '-')
		{
			throw std::runtime_error(
				"Unrecognized argument '" + arg + "' "
				"All arguments must start with '--' "
				"Use --Section.key=value to override a config value "
				"or run --help to see all available options.");
		}

		if(tryParseLibProvidedCLIFlags(arg, result.flags))
			continue;

		parseIntoParsedCLIArgs(arg,
				arg.substr(2),
				schemaLookup,
				keyMap,
				flatEnabled,
				result.values);
	}

	return result;
}

std::string preParseArgsForConfigPath(int argc, char* argv[])
{
	std::string result;
	const std::string configPrefix = "--config=";
	for (int i = 0; i < argc; ++i)
	{
		auto arg = std::string(argv[i]);
		if(arg.starts_with(configPrefix))
		{
			result = arg.substr(configPrefix.size());
			if(result.empty())
				throw std::runtime_error(
					"--config= requires a file path. Example: --config=my_settings.ini");
			break;
		}
	}
	return result;
}

} // namespace Internal
} // namespace ConfigLib
