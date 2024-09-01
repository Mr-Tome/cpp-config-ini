#include "config_library/config_reader.hpp"
#include <iostream>
#include <fstream>
#include <memory>

class SpecificAlgorithmConfig : public ConfigReader {
	
public:
    SpecificAlgorithmConfig() : ConfigReader() {
        std::cout << "SpecificAlgorithmConfig constructor started" << std::endl;
        std::cout << "SpecificAlgorithmConfig constructor finished" << std::endl;
    }

    std::string getConfigFilePath() const override {
        std::cout << "Getting config file path" << std::endl;
        return "specific_algorithm_config.ini";
    }

    const ConfigGen::ConfigSection* getConfigSections(size_t& sectionCount) const override {
        static const ConfigGen::ConfigItem ABT_ITEMS[] = {
            {"kor", "double", "500.0", "ABT kor value", "Must be positive"},
            {"koh", "double", "1.0", "ABT koh value", ""}
        };

        static const ConfigGen::ConfigItem TBM_ITEMS[] = {
            {"kor", "double", "500.0", "TBM kor value", "Must be positive"}
        };

        static const ConfigGen::ConfigItem GENERAL_ITEMS[] = {
            {"FW", "double", "10.0", "Fixed Wing value", ""},
            {"RW", "double", "20.0", "Rotary Wing value", ""},
            {"CM", "double", "30.0", "Cruise Missile value", ""},
            {"Misc", "vector<double>", "1.0,2.0,3.0", "Misc item just for proof of principle", ""}
        };

        static const ConfigGen::ConfigSection SECTIONS[] = {
            {"ABT", ABT_ITEMS, sizeof(ABT_ITEMS) / sizeof(ABT_ITEMS[0])},
            {"TBM", TBM_ITEMS, sizeof(TBM_ITEMS) / sizeof(TBM_ITEMS[0])},
            {"General", GENERAL_ITEMS, sizeof(GENERAL_ITEMS) / sizeof(GENERAL_ITEMS[0])}
        };

        sectionCount = sizeof(SECTIONS) / sizeof(SECTIONS[0]);
        return SECTIONS;
    }
protected:
    void setDefaultValues() override {
        // This method will now only be used for runtime initialization
        // The actual default values are defined in getConfigSections
		//std::cout << "SpecificAlgorithmConfig::setDefaultValues started" << std::endl;
        //try {
        //    std::cout << "Setting ABT.kor" << std::endl;
        //    setDefaultValue("ABT", "kor", 500.0);
        //    std::cout << "Setting ABT.koh" << std::endl;
        //    setDefaultValue("ABT", "koh", 1.0);
        //    std::cout << "Setting TBM.kor" << std::endl;
        //    setDefaultValue("TBM", "kor", 500.0);
        //    std::cout << "Setting General.FW" << std::endl;
        //    setDefaultValue("General", "FW", 10.0);
        //    std::cout << "Setting General.RW" << std::endl;
        //    setDefaultValue("General", "RW", 20.0);
        //    std::cout << "Setting General.CM" << std::endl;
        //    setDefaultValue("General", "CM", 30.0);
        //    std::cout << "Setting General.Factors" << std::endl;
        //    setDefaultValue("General", "Factors", std::vector<double>{1.0, 2.0, 3.0});
		//
        //    std::cout << "Setting validation rule for ABT.kor" << std::endl;
		//	setValidationRule("ABT", "kor", [](const ConfigValue& value) {
		//		const TypedConfigValue<double>* typed_value = dynamic_cast<const TypedConfigValue<double>*>(&value);
		//		return typed_value && typed_value->getValue() > 0;
		//	});
        //} catch (const std::exception& e) {
        //    std::cerr << "Exception in SpecificAlgorithmConfig::setDefaultValues: " << e.what() << std::endl;
        //    throw;
        //} catch (...) {
        //    std::cerr << "Unknown exception in SpecificAlgorithmConfig::setDefaultValues" << std::endl;
        //    throw;
        //}
        //std::cout << "SpecificAlgorithmConfig::setDefaultValues finished" << std::endl;
    }
};

// This will force the compiler to generate the config file at compile time
static const bool configGenerated = (generateConfigFile(SpecificAlgorithmConfig()), true);

 
int main() {
    std::cout << "Program started" << std::endl;
    
    try {
        std::cout << "Creating SpecificAlgorithmConfig" << std::endl;
        SpecificAlgorithmConfig config;
        config.initialize();
        
        // Print out the loaded configuration
        std::cout << "Loaded Configuration:" << std::endl;
        for (const auto& section : config.getSections()) {
            std::cout << "[" << section.first << "]" << std::endl;
            for (const auto& item : section.second.getValues()) {
                std::cout << item.first << " = " << item.second->toString() << std::endl;
            }
            std::cout << std::endl;
        }
        
        // Use the config object here
        try {
            std::cout << "ABT.kor: " << config.getValue<double>("ABT", "kor") << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error retrieving ABT.kor: " << e.what() << std::endl;
        }
        
        try {
            std::cout << "TBM.kor: " << config.getValue<double>("TBM", "kor") << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error retrieving TBM.kor: " << e.what() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "Program finished" << std::endl;
    std::cout.flush();  // Ensure all output is flushed
    
    return 0;
}
