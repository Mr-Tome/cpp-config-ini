#pragma once

#include "config_reader_base.hpp"
#include "PersistenceReaders/IPersistenceReader.hpp"

namespace ConfigLib 
{

const std::string& iniInstructions();

std::string generateConfig(const std::vector<ConfigSection>& sections); // TODO (IHT 20260308) i think i should make this virtual apart of a new PersistenceReader base class...

void generateConfigFileIfNeeded(
	const std::string& filePath,
	const std::vector<ConfigSection>& configSections);
	

void loadConfigFromFile(
	const std::string& filePath,
	const std::vector<ConfigSection>& configSections, // from derived type 
	std::unordered_map<std::string, ConfigSectionStore>& sections);//from store	
	
//only thing this should be doing is calling derived class initialize and saveConfig
template<typename Derived, bool HasCLI = false>
class INIConfigReader : public ConfigReaderBase, public IPersistenceReader
{
	
public:
	INIConfigReader()
	{
		const Derived& d = static_cast<Derived&>(*this);
		if(!HasCLI)
			assertNoVolatileFieldsInINIOnlyReader(d.getConfigSections());
		generateConfigFileIfNeeded(d.getConfigFilePath(), d.getConfigSections());
		this->filepath = d.getConfigFilePath();
		initialize(d.getConfigSections());
		loadConfigFromFile(this->filepath, d.getConfigSections(), this->sections);
	}
	
	void saveConfig() const
	{
		const Derived& d = static_cast<const Derived&>(*this);
		ConfigReaderBase::saveConfig(d.getConfigSections(),
									 this->filepath,
									 iniInstructions());
	}
	
	void persistSave(const std::vector<ConfigSection>& configSections) const override
	{
		ConfigReaderBase::saveConfig(
			configSections,
			this->filepath,
			iniInstructions());
	}

	void persistReset() override
	{
		if (std::remove(this->filepath.c_str()) != 0)
		{
			throw std::runtime_error(
				"--reset: failed to delete config file: " + this->filepath
				+ ". Does it exist?");
		}
		std::cout << "--reset: deleted '" << this->filepath
		          << "'. Defaults will regenerate on the next run." << std::endl;
	}

	void persistExport(
		const std::string& exportPath,
		const std::vector<ConfigSection>& configSections) const override
	{
		ConfigReaderBase::saveConfig(
			configSections,
			iniInstructions(),
			exportPath);
		std::cout << "--export: wrote config to '" << exportPath << "'." << std::endl;
	}
};

} // namespace ConfigLib
