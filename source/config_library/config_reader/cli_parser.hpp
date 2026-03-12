#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
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

// rawArgs[0] is argv[0] and is skipped.
// throws std::runtime_error if something is wrong with the CLI.
ParsedCLIArgs parseCLIArgs(
	const std::vector<std::string>& rawArgs,
	const std::vector<ConfigSection>& sections);
} // namespace ConfigLib
