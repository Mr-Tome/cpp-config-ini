#include "config_library/config_reader.hpp"
#include "config_library/validation_rules.hpp"
#include <iostream>
#include <fstream>
#include <memory>

class SpecificAlgorithmConfig : public ConfigLib::ConfigReader {
public:
    SpecificAlgorithmConfig() : ConfigReader() {
        std::cout << "SpecificAlgorithmConfig constructor started" << std::endl;
		initialize();
        std::cout << "SpecificAlgorithmConfig constructor finished" << std::endl;
    }

    std::string getConfigFilePath() const override {
        std::cout << "Getting config file path" << std::endl;
        return "specific_algorithm_config.ini";
    }

    std::vector<ConfigLib::ConfigGen::ConfigSection> getConfigSections() const override 
    {
        static const ValidationRules::BetweenValues between0And100(0, 100);
        return {
            {
                "ABT",
                {
                    {"kor", "double", "500.0", "ABT kor value", &ValidationRules::greaterThanZero},
                    {"koh", "integer", "1", "ABT koh value", nullptr}
                }
            },
            {
                "TBM",
                {
                    {"kor", "double", "500.0", "TBM kor value", &ValidationRules::greaterThanZero}
                }
            },
            {
                "General",
                {
                    {"FW", "double", "10.0", "Fixed Wing value", &between0And100},
                    {"RW", "double", "20.0", "Rotary Wing value", &between0And100},
                    {"CM", "double", "30.0", "Cruise Missile value", &between0And100},
                    {"Misc", "vector<double>", "1.0,2.0,3.0", "Misc item just for proof of principle", nullptr}
                }
            }
        };
    }
};
 
int main() {
    std::cout << "Program started" << std::endl;
    
    try {
        std::cout << "Creating SpecificAlgorithmConfig" << std::endl;
        SpecificAlgorithmConfig config;
        //config.initialize();
		
        // Print out the loaded configuration
        std::cout << "Loaded Configuration:" << std::endl;
        for (const auto& section : config.getSections()) {
            std::cout << "[" << section.first << "]" << std::endl;
            for (const auto& item : section.second.getValues()) {
                std::cout << item.first << " = " << item.second->toString() << std::endl;
            }
            std::cout << std::endl;
        }
        
		 // Use the config object like this
        try {
            std::cout << "1ABT.kor: " << config.getValue<double>("ABT", "kor") << std::endl;
			std::cout << "1ABT.koh: " << config.getValue<int>("ABT", "koh") << std::endl;
            //std::cout << "1TBM.kor: " << config.getValue<double>("TBM", "kor") << std::endl;
            //std::cout << "1General.FW: " << config.getValue<double>("General", "FW") << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error retrieving value: " << e.what() << std::endl;
        }
		
		// Use the config object like this
		double kor_new = config.getValue<double>("ABT", "kor") * 3.0;
		std::cout << "kor_new: " << std::to_string(kor_new) << std::endl;
		
        // Testing validation logic
        try {
            config.setValue("General", "FW", 20.0);
        } catch (const std::exception& e) {
            std::cerr << "Validation error: " << e.what() << std::endl;
        }

        // Use the config object like this
        try {
            std::cout << "2ABT.kor: " << config.getValue<double>("ABT", "kor") << std::endl;
            std::cout << "2TBM.kor: " << config.getValue<double>("TBM", "kor") << std::endl;
            std::cout << "2General.FW: " << config.getValue<double>("General", "FW") << std::endl;
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
