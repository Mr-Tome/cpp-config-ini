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
    
    const std::unordered_map<std::string, ConfigSectionStore>& getSections() const { return sections; }
    
protected:
	//should just validate schema really...
	void initialize(const std::vector<ConfigSection>& configSections);
	
	// Persistence path
    void saveConfig(const std::vector<ConfigSection>& configSections,
					const std::string& instructions_footer = "") const;
					
	//TODO (IHT 20260228): Determine how this and the function above should be handled. this is a INI/Persistence only concern.
	void assertNoVolatileFieldsInINIOnlyReader(const std::vector<ConfigSection>& configSections) const;
	//TODO (IHT 20260227): Determine how this and the function above should be handled. this is a CLI conly concern.
	void initializeForCLI(const std::vector<ConfigSection>& configSections);
    
    void setValidationRule(const std::string& section, const std::string& key, const ValidationRules::Rule* rule);
    
    void useDefaultValue(const std::string& section, const std::string& key, const ConfigItem& item);
    
    std::string filepath;
    std::unordered_map<std::string, ConfigSectionStore> sections;

private:
    void setValidationRules(const std::vector<ConfigSection>& configSections);
};

namespace Internal
{
	//cant really put this in the utility file without fwd decl
void applyDefaultValue(
    const std::string& sectionName,
    const std::string& key,
    const ConfigItem& item,
    std::unordered_map<std::string, ConfigSectionStore>& sections);
}; // namespace ConfigLib::Internal


} // namespace ConfigLib
