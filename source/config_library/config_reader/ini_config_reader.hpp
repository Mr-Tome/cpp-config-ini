#pragma once

#include "config_reader_base.hpp"

namespace ConfigLib 
{

const std::string& iniInstructions();

std::string generateConfig(const std::vector<ConfigSection>& sections); // TODO (IHT 20260308) i think i should make this virtual apart of a new PersistenceReader base class...

void generateConfigFileIfNeeded(
	const std::string& filePath,
	const std::vector<ConfigSection>& configSections);
	
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
		initialize(d.getConfigFilePath(), d.getConfigSections());
	}
	
	INIConfigReader(int argc, char* argv[])
	{
		Derived& d = static_cast<Derived&>(*this);
		for(int i = 0; i < argc; ++i) rawCLIArgs.emplace_back(argv[i]);
		
		generateConfigFileIfNeeded(d.getConfigFilePath(), d.getConfigSections());
		initialize(d.getConfigFilePath(), d.getConfigSections());
	}
	
	void saveConfig() const
	{
		const Derived& d = static_cast<const Derived&>(*this);
		ConfigReaderBase::saveConfig(d.getConfigSections(),
									 iniInstructions());
	}
};

} // namespace ConfigLib
