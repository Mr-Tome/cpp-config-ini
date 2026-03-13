#pragma once

#include "config_reader_base.hpp"

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
template<typename Derived>
class INIConfigReader : public ConfigReaderBase
{
	
public:
	INIConfigReader()
	{
		Derived& d = static_cast<Derived&>(*this);
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
									 iniInstructions());
	}
};

} // namespace ConfigLib
