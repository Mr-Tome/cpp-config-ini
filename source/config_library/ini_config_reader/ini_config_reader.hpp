#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <sstream>
#include <iostream>
#include "../common/config_value.hpp" // TODO (IHT 20260223): I think this is only needed for the ConfigSectionStore...seems like an odd place for a store..
#include "../common/config_schema.hpp"

namespace ConfigLib 
{
//owns all data and controls the i/o logic
class ConfigReaderBase {
public:
    ConfigReaderBase() = default;
    virtual ~ConfigReaderBase() = default;
    
	template<typename T>
	T getValue(const std::string& section, const std::string& key) const 
	{
		std::cout << "Attempting to get value for section: " << section 
				  << ", key: " << key << std::endl;
		auto sect_it = sections.find(section);
		if (sect_it != sections.end()) 
		{
			std::cout << "Section found" << std::endl;
			return sect_it->second.getValue<T>(key);
		}
		std::cout << "Section not found" << std::endl;
		throw std::runtime_error("Section not found: " + section);
	}

	template<typename T>
	void setValue(const std::string& section, 
								const std::string& key, const T& value) 
	{
		auto sect_it = sections.find(section);
		if (sect_it == sections.end()) {
			throw std::runtime_error("Attempting to set a Section that was not found in the schema: " + section);
		}
		
		sect_it->second.setValue(key, value);
	}


    void saveConfig(const std::vector<ConfigSection>& configSections) const;
    
    const std::unordered_map<std::string, ConfigSectionStore>& getSections() const { return sections; }
    
protected:
	void initialize(const std::string& filePath,
					const std::vector<ConfigSection>& configSections);

	//TODO (IHT 20260227): Determine how this and the function above should be handled.
	void initializeForCLI(const std::vector<ConfigSection>& configSections);
    
    void setValidationRule(const std::string& section, const std::string& key, const ValidationRules::Rule* rule);
    
    std::string filepath;
    std::unordered_map<std::string, ConfigSectionStore> sections;
	
private:
    void loadConfig(const std::vector<ConfigSection>& configSections);
    void setValidationRules(const std::vector<ConfigSection>& configSections);
    void useDefaultValue(const std::string& section, const std::string& key, const ConfigItem& item);
};

//only thing this should be doing is calling derived class initialize and saveConfig
template<typename Derived>
class INIConfigReader : public ConfigReaderBase
{
public:
	INIConfigReader()
	{
		Derived& d = static_cast<Derived&>(*this);
		initialize(d.getConfigFilePath(), d.getConfigSections());
	}
	
	void saveConfig() const
	{
		const Derived& d = static_cast<const Derived&>(*this);
		ConfigReaderBase::saveConfig(d.getConfigSections());
	}
};

std::string generateConfig(const std::vector<ConfigSection>& sections); // i think i should make this virtual and apart of ConfigReaderBase.
} // namespace ConfigLib
