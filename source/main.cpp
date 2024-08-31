#include "config_library/config_reader.hpp"
#include <iostream>
#include <memory>
#include <string>

#include <unordered_map>
#include <functional>

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>

class SpecificAlgorithmConfig : public ConfigReader {
	
public:
    SpecificAlgorithmConfig() : ConfigReader() {
        std::cout << "SpecificAlgorithmConfig constructor started" << std::endl;
        std::cout << "SpecificAlgorithmConfig constructor finished" << std::endl;
    }
protected:
    void setDefaultValues() override {
        std::cout << "SpecificAlgorithmConfig::setDefaultValues started" << std::endl;
        try {
            std::cout << "Setting ABT.kor" << std::endl;
            setDefaultValue("ABT", "kor", 500.0);
            std::cout << "Setting ABT.koh" << std::endl;
            setDefaultValue("ABT", "koh", 1.0);
            std::cout << "Setting TBM.kor" << std::endl;
            setDefaultValue("TBM", "kor", 500.0);
            std::cout << "Setting General.FW" << std::endl;
            setDefaultValue("General", "FW", 10.0);
            std::cout << "Setting General.RW" << std::endl;
            setDefaultValue("General", "RW", 20.0);
            std::cout << "Setting General.CM" << std::endl;
            setDefaultValue("General", "CM", 30.0);
            std::cout << "Setting General.Factors" << std::endl;
            setDefaultValue("General", "Factors", std::vector<double>{1.0, 2.0, 3.0});

            std::cout << "Setting validation rule for ABT.kor" << std::endl;
			setValidationRule("ABT", "kor", [](const ConfigValue& value) {
				const TypedConfigValue<double>* typed_value = dynamic_cast<const TypedConfigValue<double>*>(&value);
				return typed_value && typed_value->getValue() > 0;
			});
        } catch (const std::exception& e) {
            std::cerr << "Exception in SpecificAlgorithmConfig::setDefaultValues: " << e.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "Unknown exception in SpecificAlgorithmConfig::setDefaultValues" << std::endl;
            throw;
        }
        std::cout << "SpecificAlgorithmConfig::setDefaultValues finished" << std::endl;
    }

    std::string getConfigFilePath() const override {
        std::cout << "Getting config file path" << std::endl;
        return "specific_algorithm_config.ini";
    }
};

class SpecificAlgorithm {
private:
    std::unique_ptr<ConfigReader> config;

public:
    SpecificAlgorithm() {
        std::cout << "SpecificAlgorithm constructor started" << std::endl;
        try {
            config.reset(new SpecificAlgorithmConfig());
            std::cout << "SpecificAlgorithmConfig created" << std::endl;
            config->initialize();
            std::cout << "Config initialized" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception in SpecificAlgorithm constructor: " << e.what() << std::endl;
            throw;
        }
        std::cout << "SpecificAlgorithm constructor finished" << std::endl;
    }

    void run() {
        std::cout << "Running Specific Algorithm with configuration:\n";
        std::cout << "ABT.kor: " << config->getValue<double>("ABT", "kor") << "\n";
        std::cout << "ABT.koh: " << config->getValue<double>("ABT", "koh") << "\n";
        std::cout << "TBM.kor: " << config->getValue<double>("TBM", "kor") << "\n";
        std::cout << "FW: " << config->getValue<double>("General", "FW") << "\n";
        std::cout << "RW: " << config->getValue<double>("General", "RW") << "\n";
        std::cout << "CM: " << config->getValue<double>("General", "CM") << "\n";
        
        auto factors = config->getValue<std::vector<double>>("General", "Factors");
        std::cout << "Factors: ";
        for (const auto& factor : factors) {
            std::cout << factor << " ";
        }
        std::cout << "\n";
    }

    ConfigReader* getConfig() { return config.get(); }
};

int main() {
    std::cout << "Program started" << std::endl;
    
    try {
        std::cout << "Creating SpecificAlgorithm" << std::endl;
        SpecificAlgorithm algorithm;
        
        std::cout << "Running algorithm" << std::endl;
        algorithm.run();

        std::cout << "Modifying a value and saving" << std::endl;
        auto* config = dynamic_cast<SpecificAlgorithmConfig*>(algorithm.getConfig());
        if (config) {
            config->setValue("ABT", "kor", 600.0);
            config->saveConfig();

            std::cout << "Configuration saved. Re-running algorithm:" << std::endl;
            algorithm.run();
        } else {
            std::cout << "Failed to cast config to SpecificAlgorithmConfig" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "Program finished" << std::endl;
    std::cout.flush();  // Ensure all output is flushed
    
    return 0;
}