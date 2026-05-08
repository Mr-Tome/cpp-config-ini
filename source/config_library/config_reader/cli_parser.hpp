#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include "../common/config_schema.hpp"

namespace ConfigLib
{
namespace Internal
{

struct LibProvidedCLIFlags
{
	bool help = false;
	bool print = false;
	bool save = false;
	bool _delete = false;
	bool diff = false;

	bool schema_dry_run = false;
	bool schema_version = false;

	std::string flat;
	std::string config_path;
	std::string export_path;
};

struct ParsedCLIArgs
{
	std::map<std::pair<std::string, std::string>, std::string> values;
	LibProvidedCLIFlags flags;
};

struct CLIKeyMap
{
	std::map<std::string, std::pair<std::string, std::string>> flatToQualified;
	std::map<std::pair<std::string, std::string>, std::string> qualifiedToFlat;
	std::set<std::string> ambiguousKeysWhenFlat;
};

CLIKeyMap buildCLIKeyMap(const std::vector<ConfigSection>& sections);

ParsedCLIArgs parseCLIArgs(
	const std::vector<std::string>& rawArgs,
	const std::vector<ConfigSection>& sections,
	const CLIKeyMap& keyMap,
	bool flatEnabled);

std::string preParseArgsForConfigPath(int argc, char* argv[]);

} // namespace Internal
} // namespace ConfigLib
