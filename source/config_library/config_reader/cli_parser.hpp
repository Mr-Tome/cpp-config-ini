#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include "../common/config_schema.hpp"

namespace ConfigLib
{
struct LibProvidedCLIFlags
{
	bool help = false; // prints out how CLI instructions 
	bool print = false; // print outs the configuration 
	bool save = false; // writes current state back to the Persistence Reader (INI/JSON/etc.)
	bool reset = false; // deliete the INI file so it regenerates from the default.
	bool diff = false; // show the values that differ from the schema defaults
	
	
	std::string flat; // enables a flat key lookup
	std::string config_path; //use an alterante INI file path.
	std::string export_path; //write current config to a new file at this path.
	
};

struct ParsedCLIArgs
{
	// parsed {section, key} pairs to their raw parsed string values
	std::map<std::pair<std::string, std::string>, std::string> values;
	LibProvidedCLIFlags flags;
};

struct CLIKeyMap
{
	//flat key name to {section name, key}
	std::map<std::string, std::pair<std::string, std::string>> flatToQualified;
	
	//{section name, key} to flat key name
	std::map<std::pair<std::string, std::string>, std::string> qualifiedToFlat;
	
	//when flattened names resolve to the same name, we have no choice
	// but to require the fully qualified form regardless of flat setting.
	std::set<std::string> ambiguousKeysWhenFlat;
};

CLIKeyMap buildCLIKeyMap(const std::vector<ConfigSection>& sections);

// rawArgs[0] is argv[0] and is skipped.
// throws std::runtime_error if something is wrong with the CLI.
ParsedCLIArgs parseCLIArgs(
	const std::vector<std::string>& rawArgs,
	const std::vector<ConfigSection>& sections,
	const CLIKeyMap& keyMap,
	bool flatEnabled);
	
std::string preParseArgsForConfigPath(int argc, char* argv[]);
} // namespace ConfigLib
