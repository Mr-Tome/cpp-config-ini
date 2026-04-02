#pragma once
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include "config_reader_base.hpp"
#include "PersistenceReaders/IPersistenceReader.hpp"
#include "../common/schema_evolver.hpp"

namespace ConfigLib 
{

const std::string& iniInstructions();

void loadConfigFromFile(
	const std::string& filePath,
	const std::vector<ConfigSection>& configSections, // from derived type 
	std::unordered_map<std::string, ConfigSectionStore>& sections);//from store	

bool parseRawINI(
    const std::string& filePath,
    RawConfigMap& rawConfig);

	
//only thing this should be doing is calling saveConfig
template<typename Derived, bool HasCLI = false>
class INIConfigReader : public ConfigReaderBase, public IPersistenceReader
{
	void init(const Derived& d,
			  const std::string& configFilePath)
	{
		auto mergedSections = mergeDuplicateSections(d.getConfigSections());
		
		if(!HasCLI)
			assertNoVolatileFieldsInINIOnlyReader(mergedSections);
			
		this->filepath = configFilePath.empty() ? d.getConfigFilePath() : configFilePath;
		initialize(mergedSections);
		generateConfigFileIfNeeded(this->filepath, mergedSections);
        runSchemaEvolution(d, mergedSections);
		loadConfigFromFile(this->filepath, mergedSections, this->sections);
	}
public:
	INIConfigReader()
	{
		const Derived& d = static_cast<Derived&>(*this);
		init(d, d.getConfigFilePath());
	}
	
	explicit INIConfigReader(const std::string& configFilePathOverride)
	{
		init(static_cast<Derived&>(*this), configFilePathOverride);
	}
	
	// derived can override this
	OrphanedConfigItemPolicy getOrphanedConfigItemPolicy() const
    {
        return OrphanedConfigItemPolicy::CommentOut;
    }
	
	void saveConfig() const
	{
		const Derived& d = static_cast<const Derived&>(*this);
		auto mergedSections = mergeDuplicateSections(d.getConfigSections());
		writeToFile(mergedSections, this->filepath);
	}
	
	void persistSave(const std::vector<ConfigSection>& configSections) const override
	{
		writeToFile(configSections, this->filepath);
	}

	void persistDelete() override
	{
		if (std::remove(this->filepath.c_str()) != 0)
		{
			throw std::runtime_error(
				"--delete: failed to delete config file: " + this->filepath
				+ ". Does it exist?");
		}
		std::cout << "--delete: deleted '" << this->filepath
		          << "'. Defaults will regenerate on the next run." << std::endl;
	}

	void persistExport(
		const std::string& exportPath,
		const std::vector<ConfigSection>& configSections) const override
	{
		writeToFile(configSections, exportPath);
		std::cout << "--export: wrote config to '" << exportPath << "'." << std::endl;
	}
private:

void runSchemaEvolution(
	const Derived& d,
	const std::vector<ConfigSection>& mergedSections)
{
}

void generateConfigFileIfNeeded(
	const std::string& filePath,
	const std::vector<ConfigSection>& mergedSections)
{
	std::ifstream file(filePath);
	
	// TODO (IHT): Update to boost::filesystem::exists(filePath)
	if (file.is_open()) {
		std::cout << "Configuration file already exists. Skipping generation." << std::endl;
		return;
	}

	if (!validateConfig(mergedSections)) 
	{
		throw std::runtime_error("Invalid configuration detected at runtime");
	}

	std::string configContent = formatINI(mergedSections,
			[](const ConfigSection&, const ConfigItem& item)
            {
                return item.defaultValue;
            });

	std::ofstream configFile(filePath);
	if (configFile.is_open()) 
	{
		configFile << configContent;
		configFile.close();
	} 
	else 
	{
		throw std::runtime_error("Unable to open file for writing: " + filePath);
	}
}

void writeToFile(
        const std::vector<ConfigSection>& configSections,
        const std::string& path) const
{
	std::cout << "Saving the current configuration to: " << path << std::endl;

	std::string content = formatINI(
		configSections,
		[this](const ConfigSection& section, const ConfigItem& item)
		{
			return this->sections.at(section.name).getValues().at(item.name)->toString();
		});

	std::ofstream out(path);
	if (!out.is_open())
		throw std::runtime_error("Unable to open file for writing: " + path);
	out << content;

	std::cout << "Finished saving configuration to: " << path << std::endl;
}

std::string formatINI(
	const std::vector<ConfigSection>& configSections,
	std::function<std::string(const ConfigSection&, const ConfigItem&)> valueSource) const
{
	std::string config_content = "# Configuration file\n\n";

	for (const auto& section : configSections) 
	{
		config_content += "[" + section.name + "]\n";
		for (const auto& item : section.items) 
		{
			if (item.persistence == Persistence::Volatile)
			{
				config_content += "# " + item.name + " = <not stored>"
					+ " # type: " + item.type
					+ ", description: " + item.description
					+ " [VOLATILE: supply via CLI every run]\n";
				continue;
			}
			config_content += item.name + " = " + valueSource(section, item)
				+ " # type: " + item.type
				+ ", description: " + item.description;
			if (item.validationRule)
				config_content += " (validationRule: " + item.validationRule->toString() + ")";
			
			config_content += "\n";
		}
		config_content += "\n";
	}

	config_content += iniInstructions();

	return config_content;
}
};

} // namespace ConfigLib
