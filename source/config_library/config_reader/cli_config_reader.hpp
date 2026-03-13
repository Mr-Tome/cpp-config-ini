#pragma once
#include "ini_config_reader.hpp"
#include "cli_parser.hpp"

namespace ConfigLib
{

// current only used for CLI only persistence base. no file i/o
// TODO (IHT 20260307: consider extending for jsons)	
template<typename Derived>
class NoPersistenceReader: public ConfigReaderBase
{
public:
	NoPersistenceReader()
	{
		Derived& d = static_cast<Derived&>(*this);
		initializeForCLI(d.getConfigSections());
	}
};

//cli wrapper class. 
//Intended to be the place where different CLI features are introduced...
template<typename Derived, typename Base>
class CLIFeatureLayer : public Base
{
public:
	CLIFeatureLayer() : Base()
	{
		std::cout << "CLIFeatureLayer default constructor called" << std::endl;
		applyCLIOverrides();
	}
	
	CLIFeatureLayer(int argc, char* argv[]) : Base()
	{
		std::cout << "CLIFeatureLayer argc & argv constructor called" << std::endl;
		for (int i = 0; i < argc; ++i) rawCLIArgs.emplace_back(argv[i]);
		applyCLIOverrides();
	}
	
	void printRawArgs()
	{
		std::cout << "------rawCLIArgs-----" << std::endl;
		std::cout << "Number of Args: " << rawCLIArgs.size() <<std::endl;
		std::cout << "values: " << std::endl;
		for(const auto& arg: rawCLIArgs)
		{
			std::cout << "------"<<arg<<"------"<<std::endl;
		}
	}
private:
	std::vector<std::string> rawCLIArgs;
	
	void applyCLIOverrides()
	{
		Derived& d = static_cast<Derived&>(*this);
		const auto configSections = d.getConfigSections();
		const ParsedCLIArgs parsed = parseCLIArgs(this->rawCLIArgs,
														configSections);
		
		if(parsed.values.empty())
			return;
		
		std::unordered_map<std::string,
			std::unordered_map<std::string, const ConfigItem*>> schemaLookup;
			
		for (const auto& section : configSections)
			for (const auto& item : section.items)
				schemaLookup[section.name][item.name] = &item;
				
		auto& registry = TypeRegistry::instance();
		for (const auto& kv : parsed.values)
		{
			const std::string& section = kv.first.first;
			const std::string& key     = kv.first.second;
			const std::string& value   = kv.second;

			const ConfigItem& item = *schemaLookup.at(section).at(key);

			try
			{
				auto parsedValue = registry.parseValue(item.type, value);
				
				if (item.validationRule && !(*item.validationRule)(*parsedValue))
				{
					throw std::runtime_error(
						"Value '" + value + "' failed validation rule: "
						+ item.validationRule->toString());
				}

				this->sections.at(section).getValues()[key] = parsedValue;
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error(
					"Error applying CLI override --" + section + "." + key
					+ "=" + value + ": " + e.what());
			}
		}

		std::cout << "CLIFeatureLayer: applied " << parsed.values.size()
		          << " CLI override(s)" << std::endl;
	}
};

} // namespace ConfigLib
