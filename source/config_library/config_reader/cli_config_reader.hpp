#pragma once
#include <type_traits>
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
		auto mergedSections = mergeDuplicateSections(d.getConfigSections());
		validateForCLI(mergedSections);
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
		init();
	}
	
	CLIFeatureLayer(int argc, char* argv[]) : Base()
	{
		std::cout << "CLIFeatureLayer argc & argv constructor called" << std::endl;
		for (int i = 0; i < argc; ++i) rawCLIArgs.emplace_back(argv[i]);
		init();
	}
	
	bool flattenCLIArgs() const {return true;}

private:
	std::vector<std::string> rawCLIArgs;
	
	using HasPersistence = std::is_base_of<IPersistenceReader, Base>;
	
	void handleSave(
		const std::vector<ConfigSection>& configSections,
		std::true_type) const
	{
		static_cast<const IPersistenceReader*>(this)
			->persistSave(withoutVolatileItems(configSections));
	}
	void handleSave(
		const std::vector<ConfigSection>&,
		std::false_type) const
	{
		std::cerr << "Warning: --save has no effect because no persistence layer was configured.\n";
	}

	void handleReset(std::true_type)
	{
		static_cast<IPersistenceReader*>(this)->persistReset();
	}
	void handleReset(std::false_type)
	{
		std::cerr << "Warning: --reset has no effect because no persistence layer configured.\n";
	}
	
	void handleExport(
		const std::string& exportPath,
		const std::vector<ConfigSection>& configSections,
		std::true_type) const
	{
		static_cast<const IPersistenceReader*>(this)
			->persistExport(exportPath, withoutVolatileItems(configSections));
	}
	void handleExport(
		const std::string&,
		const std::vector<ConfigSection>&,
		std::false_type) const
	{
		std::cerr << "Warning: --export has no effect because no persistence layer configured.\n";
	}
	
	
	void printPersistenceFlags(std::true_type) const
	{
		std::cout
			<< "  --save                            Write current state to config file (volatile fields skipped)\n"
			<< "  --reset                           Delete config file; defaults regenerate on next run\n"
			<< "  --export=<path>                   Write the config to a new file\n"
			<< "  --config=<path>                   Use an alternative config file\n";
	}
	void printPersistenceFlags(std::false_type) const {}
	
	bool isFlatEnabled(const Derived& d)
	{
		bool flatEnabled = d.flattenCLIArgs();
		const std::string flatPrefix = "--flat=";
		
		for(const auto& arg : this->rawCLIArgs)
		{
			if(arg.size() > flatPrefix.size() &&
				arg.substr(0,flatPrefix.size())==flatPrefix)
			{
				const std::string val = arg.substr(flatPrefix.size());
				try
				{
					flatEnabled = TypeParser<bool>::fromString(val);
				}
				catch(...){}
				break;
			}
		}
		return flatEnabled;
	}
	
	void init()
	{
		const Derived& d = static_cast<Derived&>(*this);
		const auto configSections = mergeDuplicateSections(d.getConfigSections());
		
		const CLIKeyMap keyMap = buildCLIKeyMap(configSections);
		
		bool flatEnabled = isFlatEnabled(d);
		
		const ParsedCLIArgs parsed = parseCLIArgs(this->rawCLIArgs,
														configSections,
														keyMap,
														flatEnabled);
		
														
		helpIfNeeded(configSections, parsed, keyMap, flatEnabled);
		applyCLIOverrides(configSections,parsed);
		
		//(IHT 2026.03.18) This needs to go last in this constructor atm,
		// because it's acting on a fully resolved stated 
		// of this class and base classes
		parsePostConstructionSystemFlags(configSections, parsed);
	}
	
	
	void parsePostConstructionSystemFlags(
		const std::vector<ConfigSection>& configSections,
		const ParsedCLIArgs& parsed)
	{
		// TODO (IHT 2026.03.14): --config requires overriding developers getConfigFilePath before Base() runs in the initializer list.
		// but rawCLIArgs isnt popuilated/parsed until constructor body. have to figure out how to defer...
		// also, config_path is really only a Persistence store thing...
		if (!parsed.flags.config_path.empty())
			std::cerr << "Warning: --config= is not yet supported and has been ignored.\n";
	
		if (parsed.flags.print)
			printConfig(configSections);
		
		if (parsed.flags.save)
			handleSave(configSections, HasPersistence{});

		if (parsed.flags.reset)
			handleReset(HasPersistence{});

		if (!parsed.flags.export_path.empty())
			handleExport(parsed.flags.export_path, configSections, HasPersistence{});

		if (parsed.flags.diff)
			printDiff(configSections);
	}
	
	void helpIfNeeded(
		const std::vector<ConfigSection>& configSections,
		const ParsedCLIArgs& parsed,
		const CLIKeyMap& keyMap,
		bool flatEnabled) const
	{
		if(parsed.flags.help)
		{
			printHelp(configSections, keyMap, flatEnabled);
			std::exit(0);
		}
	}
	
	void applyCLIOverrides(
		const std::vector<ConfigSection>& configSections,
		const ParsedCLIArgs& parsed)
	{
		std::unordered_map<std::string,
			std::unordered_map<std::string, const ConfigItem*>> schemaLookup;
			
		for (const auto& s : configSections)
			for (const auto& i : s.items)
				schemaLookup[s.name][i.name] = &i;
		
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
				this->sections.at(section).setValueFromParsed(key, parsedValue);
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
	
	// returns a copy of configSections with all volatile items stripped out.
	// used by persistSave and persistExport so volatile fields are never written
	// to any file
	static std::vector<ConfigSection> withoutVolatileItems(
		const std::vector<ConfigSection>& configSections)
	{
		std::vector<ConfigSection> filtered;
		for (const auto& section : configSections)
		{
			ConfigSection filteredSection;
			filteredSection.name = section.name;
			for (const auto& item : section.items)
			{
				if (item.persistence != Persistence::Volatile)
					filteredSection.items.push_back(item);
			}
			filtered.push_back(filteredSection);
		}
		return filtered;
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
	
	void printHelp(
		const std::vector<ConfigSection>& configSections,
		const CLIKeyMap& keyMap, 
		bool flatEnabled) const
	{
		std::string dashes = "--------------------------";
		std::string dashes2 = "------------------------";
		std::cout 
			<< dashes <<dashes <<"\n"
			<< dashes2 << "Help" <<dashes2<<"\n"
			<< dashes <<dashes <<"\n";


		const std::string programName =
			this->rawCLIArgs.empty() ? "<program>" : this->rawCLIArgs[0];
			
		std::cout << "\nProgram Usage: " << programName << " [options]\n";
		
		if (flatEnabled)
			std::cout
				<< "\nFlat lookup is ON: use --key=value as a shorthand instead of"
				   " --Section.key=value.\n"
				<< "Keys marked [qualified only], in the schema below, collide with another section and"
				   " always require --Section.key=value.\n";
		else
			std::cout
				<< "\nFlat lookup is OFF: always use --Section.key=value.\n"
				<< "Enable with --flat=true.\n";
			
		for (const auto& section : configSections)
		{
			std::cout << "\n[" << section.name <<"]\n";
			for(const auto& item:section.items)
			{
				const bool isVolatile = (item.persistence == Persistence::Volatile);
				const auto qualifiedPair = std::make_pair(section.name, item.name);
				
				std::ostringstream line;
				line << "  --" << section.name << "." <<item.name
					 << "=<" << item.type << ">";
				//std::cout << line.str();
				
				if (keyMap.ambiguousKeysWhenFlat.find(item.name) != keyMap.ambiguousKeysWhenFlat.end())
				{
					line << "  [qualified only]";
				}
				
				//line << "\n";
				
				const std::string lineStr = line.str();
				const int padTo = 42;
				const int pad = padTo - static_cast<int>(lineStr.size());
				std::cout << lineStr << std::string(pad > 0 ? pad : 1, ' ');
				
				std::cout << item.description;
				
				if(isVolatile)
					std::cout << "  [REQUIRED]";
				else
					std::cout << "  [default: " << item.defaultValue << "]";
					
				if(item.validationRule)
					std::cout << "  (" << item.validationRule->toString() << ")";
				
				std::cout << "\n";
			}
		}
		
		std::cout 
		<< "\nSystem Flags:\n"
		<< "  --help                            Print this message and exit\n"
		<< "  --print                           Dump the resolved config after all overrides\n"
		<< "  --diff                            Show values that differ from schema defaults\n"
		<< "  --flat=<bool>                     Enable or disable flat key lookup (e.g. --flat=true)"
			" (current default: "<< (flatEnabled ? "true": "false") << ")\n";
		
		printPersistenceFlags(HasPersistence{});
		std::cout << "\n";
	}
	
	void printConfig(const std::vector<ConfigSection>& configSections) const
	{
		std::cout << "\n--- Current configuration ---\n";
		for (const auto& section : configSections)
		{
			std::cout << "[" << section.name << "]\n";
			for (const auto& item : section.items)
			{
				if (item.persistence == Persistence::Volatile) continue;
				
				const std::string currentValue =
					this->sections.at(section.name).getValues().at(item.name)->toString();
				std::cout << "  " << item.name << " = " << currentValue << "\n";
			}
		}
		std::cout << "--- End configuration ---\n\n";
	}
	
	void printDiff(
		const std::vector<ConfigSection>& configSections) const
	{
		std::cout << "\n--- Diff from schema defaults ---\n";
		bool anyDiff = false;

		for (const auto& section : configSections)
		{
			for (const auto& item : section.items)
			{
				if (item.persistence == Persistence::Volatile) continue;

				const std::string currentValue =
					this->sections.at(section.name).getValues().at(item.name)->toString();

				if (currentValue != item.defaultValue)
				{
					std::cout << "  " << section.name << "." << item.name
					          << ": " << currentValue
					          << "  (default: " << item.defaultValue << ")\n";
					anyDiff = true;
				}
			}
		}

		if (!anyDiff)
			std::cout << "  (all values match schema defaults)\n";

		std::cout << "--- End diff ---\n\n";
	}
};

} // namespace ConfigLib
