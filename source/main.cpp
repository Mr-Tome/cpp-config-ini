#include "config_library/config_reader.hpp"
#include "config_library/validation_rules.hpp"
#include <iostream>
#include <fstream>
#include <memory>

class SpecificAlgorithmConfig : public ConfigReader {
public:
    SpecificAlgorithmConfig() : ConfigReader() {
        std::cout << "SpecificAlgorithmConfig constructor started" << std::endl;
		setValidationRules();
        std::cout << "SpecificAlgorithmConfig constructor finished" << std::endl;
    }

    std::string getConfigFilePath() const override {
        std::cout << "Getting config file path" << std::endl;
        return "specific_algorithm_config.ini";
    }

    const ConfigGen::ConfigSection* getConfigSections(size_t& sectionCount) const override {
        static const ConfigGen::ConfigItem ABT_ITEMS[] = {
            {"kor", "double", "500.0", "ABT kor value", &ValidationRules::greaterThanZero},
            {"koh", "double", "1.0", "ABT koh value", nullptr}
        };

        static const ConfigGen::ConfigItem TBM_ITEMS[] = {
            {"kor", "double", "500.0", "TBM kor value", &ValidationRules::greaterThanZero}
        };

        static const ValidationRules::BetweenValues EXAMPLE_RULE = ValidationRules::betweenValues(0, 100);
        static const ConfigGen::ConfigItem GENERAL_ITEMS[] = {
            {"FW", "double", "10.0", "Fixed Wing value", &EXAMPLE_RULE},
            {"RW", "double", "20.0", "Rotary Wing value", &EXAMPLE_RULE},
            {"CM", "double", "30.0", "Cruise Missile value", &EXAMPLE_RULE},
            {"Misc", "vector<double>", "1.0,2.0,3.0", "Misc item just for proof of principle", nullptr}
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
		// This method can stay empty if you want the validation rules to be explicit, i.e.,
		// presented in the generated .ini
		// implement getConfigSections for explicit rules
		
        // if you want implicit validation rules, you can se them here like this:
		// 
        //setValidationRule("ABT", "kor", ValidationRules::greaterThanZero());
        //setValidationRule("TBM", "kor", ValidationRules::greaterThanZero());
        //setValidationRule("General", "FW", ValidationRules::betweenValues(0, 100));
        //setValidationRule("General", "RW", ValidationRules::betweenValues(0, 100));
        //setValidationRule("General", "CM", ValidationRules::betweenValues(0, 100));
		//
        //// Custom validation rule example
        //setValidationRule("General", "Misc", ValidationRules::custom<std::vector<double>>([](const std::vector<double>& vec) {
        //    return vec.size() == 3 && vec[0] < vec[1] && vec[1] < vec[2];
        //}));
    }
private:
	void setValidationRules() {
        setValidationRule("ABT", "kor", ValidationRules::greaterThanZero);
        setValidationRule("TBM", "kor", ValidationRules::greaterThanZero);
        setValidationRule("General", "FW", ValidationRules::betweenValues(0, 100));
        setValidationRule("General", "RW", ValidationRules::betweenValues(0, 100));
        setValidationRule("General", "CM", ValidationRules::betweenValues(0, 100));
    }

};

// This will force generation of the config file at run time
//static const bool configGenerated = (generateConfigFile(SpecificAlgorithmConfig()), true);

 
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
        
        // Testing validation logic
        try {
            config.setValue("General", "FW", 10.0);
        } catch (const std::exception& e) {
            std::cerr << "Validation error: " << e.what() << std::endl;
        }

        // Use the config object like this
        try {
            std::cout << "ABT.kor: " << config.getValue<double>("ABT", "kor") << std::endl;
            std::cout << "TBM.kor: " << config.getValue<double>("TBM", "kor") << std::endl;
            std::cout << "General.FW: " << config.getValue<double>("General", "FW") << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error retrieving value: " << e.what() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "Program finished" << std::endl;
    std::cout.flush();  // Ensure all output is flushed
    
    return 0;
}
