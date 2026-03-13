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
		
		if(parsed.flags.help)
		{
			printHelp(configSections);
			std::exit(0);
		}
		
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
		
		checkIfVolatileItemsWereParsed(configSections, parsed);
		
		std::cout << "CLIFeatureLayer: applied " << parsed.values.size()
		          << " CLI override(s)" << std::endl;
	}
	
	void checkIfVolatileItemsWereParsed(
			const std::vector<ConfigSection>& configSections,
			const ParsedCLIArgs& parsed)
	{
		std::vector<std::string> missingVolatile;
		
		for (const auto& section : configSections)
		{
			for (const auto& item : section.items)
			{
				if(item.persistence != Persistence::Volatile) continue;
				
				if(parsed.values.find({section.name, item.name}) 
						== parsed.values.end())
				{
					missingVolatile.push_back(
						" --" + section.name + "." + item.name
						+ "=<" + item.type + "> ("+item.description+")"
					);
				}
			}
		}
		if(!missingVolatile.empty())
		{
			std::string msg = 
				"Missing required volatile field(s). "
				"These must be supplied on every run via CLI:\n";
			for(const auto& volatile_err_string : missingVolatile)
				msg+= volatile_err_string + "\n";
			
			throw std::runtime_error(msg);
		}
	}
	
	void printHelp(const std::vector<ConfigSection>& configSections)
	{
		const std::string programName =
			this->rawCLIArgs.empty() ? "<program>" : this->rawCLIArgs[0];
			
		std::cout 
			<< "\nUsage: " << programName << " [options]\n"
			<< "\n Configure the application with the following options:\n"
			<< " Example: Example: ./build/configs --Run.verbosity=2 --Run.run_id=my_run\n";
			
			
		for (const auto& section : configSections)
		{
			std::cout << "\n [" << section.name <<"]\n";
			for(const auto& item:section.items)
			{
				const bool isVolatile = (item.persistence == Persistence::Volatile);
				std::ostringstream line;
				line << " --" << section.name << "." <<item.name
					 << "=<" << item.type << ">";
				
				const std::string lineStr = line.str();
				const int padTo = 42;
				const int pad = padTo - static_cast<int>(lineStr.size());
				std::cout <<lineStr << std::string(pad > 0 ? pad : 1, ' ');
				
				std::cout << item.description;
				
				if(isVolatile)
					std::cout << "  [REQUIRED: must be supplied every run]";
				else
					std::cout << "  [default: " << item.defaultValue << "]";
					
				if(item.validationRule)
					std::cout << "  (" << item.validationRule->toString() << ")";
				
				std::cout << "\n";
			}
		}
		
		LibProvidedCLIFlags::printFlags();
	}
};

} // namespace ConfigLib
