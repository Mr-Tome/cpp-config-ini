#include "config_reader.hpp"
#include <fstream>
#include <sstream>
#include <typeinfo>
#include <iostream>
#include <stdexcept>
#include <algorithm>


namespace ConfigLib {
	namespace ConfigGen {
		// This function can be used for compile-time checks if needed
		constexpr bool validateConfigStructure() {
			// Add any compile-time checks here
			return true;
		}
	
		// Runtime validation of the configuration
		bool validateConfig(const std::vector<ConfigSection>& sections) {
			// Add runtime checks here
			return true;
		}
	
		std::string generateConfig(const std::vector<ConfigSection>& sections) {
			std::string config_content = "# Configuration file generated automatically\n\n";
	
			for (const auto& section : sections) {
				config_content += "[" + section.name + "]\n";
				for (const auto& item : section.items) {
					config_content += std::string(item.name) + " = " + std::string(item.defaultValue)
                        + " # type: " + std::string(item.type)
                        + ", description: " + std::string(item.description);
					if (item.validationRule) {
						config_content += " (validationRule: " + item.validationRule->toString() + ")";
					}
					config_content += "\n";
				}
				config_content += "\n";
			}
	
			config_content += R"(
# Instructions for End Users
# 1. Configuration File Format:
#    * The configuration file is in INI format
#    * Sections are denoted by square brackets: [SectionName]
#    * Key-value pairs are separated by an equals sign: Key = Value
#    * Comments start with a # symbol
# 2. Modifying the Configuration:
#    a. Open the configuration file (e.g., specific_algorithm_config.ini) in a text editor
#    b. Locate the section and key you want to modify
#    c. Change the value after the equals sign
#    d. Save the file
# 3. Example Configuration:
#    [Section1]
#    name1 = 0
#    name2 = 1.0
#    [Section2]
#    name3 = 500.0
#    [Section3]
#    name4 = 10.0
#    name5 = 20.0
#    name1 = 30.0
#    name2 = 1.0,2.0,3.0
# 4. Adding New Values:
#    * You can add new key-value pairs to existing sections
#    * Do not add new sections unless instructed by the developers
# 5. Value Types:
#    * Numbers can be integers or decimals (e.g., 500 or 500.0)
#    * Text should not be enclosed in quotes
#    * Lists are comma-separated (e.g., 1.0,2.0,3.0)
# 6. Validation:
#    * Some values may have validation rules (e.g., must be positive)
#    * If you enter an invalid value, the application will use the default value
# 7. Troubleshooting:
#    * If the application fails to start, check for typos in the configuration file
#    * Ensure all required keys are present
#    * If in doubt, rename or delete the configuration file to reset to defaults
# 8. Best Practices:
#    * Keep a backup of the original configuration file
#    * Document any changes you make for future reference
#    * If you're unsure about a setting, consult the application documentation or contact the developers
# Remember, incorrect configuration can affect the application's performance or cause errors.
# If you're unsure about a setting, it's best to consult with the development team or refer to the application's documentation.
)";
	
			return config_content;
		}
	}
	
	
	//TypedConfigValue implementation
	template<typename T>
	TypedConfigValue<T>::TypedConfigValue(const T& val) : value(val) {}
	
	// Explicit instantiations for the constructors
	template TypedConfigValue<int>::TypedConfigValue(const int&);
	template TypedConfigValue<double>::TypedConfigValue(const double&);
	template TypedConfigValue<std::string>::TypedConfigValue(const std::string&);
	
	
	template<typename T>
	const T& TypedConfigValue<T>::getValue() const { return value; }
	
	template<typename T>
	void TypedConfigValue<T>::setValue(const T& val) { value = val; }
	
	template<typename T>
	std::string TypedConfigValue<T>::toString() const { return std::to_string(value); }
	
	template<typename T>
	void TypedConfigValue<T>::fromString(const std::string& str) { value = static_cast<T>(std::stod(str)); }
	
	template<typename T>
	std::shared_ptr<ConfigValue> TypedConfigValue<T>::clone() const { return std::make_shared<TypedConfigValue<T>>(value); }
	
	// Specialization for std::string
	template<>
	std::string TypedConfigValue<std::string>::toString() const { return value; }
	
	template<>
	void TypedConfigValue<std::string>::fromString(const std::string& str) { value = str; }
	
	template<>
	TypedConfigValue<std::vector<double>>::TypedConfigValue(const std::vector<double>& val) : value(val) {}
	
	template<>
	const std::vector<double>& TypedConfigValue<std::vector<double>>::getValue() const { return value; }
	
	template<>
	void TypedConfigValue<std::vector<double>>::setValue(const std::vector<double>& val) { value = val; }
	
	template<>
	std::string TypedConfigValue<std::vector<double>>::toString() const {
		std::ostringstream oss;
		for (size_t i = 0; i < value.size(); ++i) {
			if (i > 0) oss << ",";
			oss << value[i];
		}
		return oss.str();
	}
	
	template<>
	void TypedConfigValue<std::vector<double>>::fromString(const std::string& str) {
		value.clear();
		std::istringstream iss(str);
		std::string item;
		while (std::getline(iss, item, ',')) {
			value.push_back(std::stod(item));
		}
	}
	
	template<>
	std::shared_ptr<ConfigValue> TypedConfigValue<std::vector<double>>::clone() const {
		return std::make_shared<TypedConfigValue<std::vector<double>>>(value);
	}
	
	template<typename T>
	void ConfigSection::setValue(const std::string& key, const T& value) {
		std::cout << "ConfigSection::setValue called for key: " << key << " with type: " << typeid(T).name() << std::endl;
		try {
			auto newValue = std::make_shared<TypedConfigValue<T>>(value);
			
			// Apply validation rule if it exists
			auto rule_it = validationRules.find(key);
			if (rule_it != validationRules.end() && rule_it->second) {
				if (!(*rule_it->second)(*newValue)) {
					std::cerr << "Validation failed for key: " << key << ". Using default value." << std::endl;
					return;
				}
			}
			
			values[key] = newValue;
			std::cout << "Value set for key: " << key << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Exception in ConfigSection::setValue: " << e.what() << std::endl;
			throw;
		} catch (...) {
			std::cerr << "Unknown exception in ConfigSection::setValue" << std::endl;
			throw;
		}
	}
	
	void ConfigSection::setValue(const std::string& key, const std::string& value) {
		std::cout << "ConfigSection::setValue called for key: " << key << " with type: string" << std::endl;
		try {
			if (values.find(key) != values.end()) {
				values[key]->fromString(value);
			} else {
				auto newValue = std::make_shared<TypedConfigValue<std::string>>(value);
				values[key] = newValue;
			}
			
			// Apply validation rule if it exists
			auto rule_it = validationRules.find(key);
			if (rule_it != validationRules.end() && rule_it->second) {
				if (!(*rule_it->second)(*values[key])) {
					throw std::runtime_error("Validation failed for key: " + key);
				}
			}
			
			std::cout << "Value set for key: " << key << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Exception in ConfigSection::setValue: " << e.what() << std::endl;
			throw;
		} catch (...) {
			std::cerr << "Unknown exception in ConfigSection::setValue" << std::endl;
			throw;
		}
	}
	
	// Specialization for vector<double>
	void ConfigSection::setValue(const std::string& key, const std::vector<double>& value) {
		std::cout << "ConfigSection::setValue called for key: " << key << " with type: vector<double>" << std::endl;
		try {
			auto newValue = std::make_shared<TypedConfigValue<std::vector<double>>>(value);
			
			// Apply validation rule if it exists
			auto rule_it = validationRules.find(key);
			if (rule_it != validationRules.end() && rule_it->second) {
				if (!(*rule_it->second)(*newValue)) {
					throw std::runtime_error("Validation failed for key: " + key);
				}
			}
			
			values[key] = newValue;
			std::cout << "Value set for key: " << key << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Exception in ConfigSection::setValue: " << e.what() << std::endl;
			throw;
		} catch (...) {
			std::cerr << "Unknown exception in ConfigSection::setValue" << std::endl;
			throw;
		}
	}
	
	template<typename T>
	T ConfigSection::getValue(const std::string& key) const {
		std::cout << "ConfigSection::getValue called for key: " << key << " with expected type: " << typeid(T).name() << std::endl;
		auto it = values.find(key);
		if (it != values.end()) {
			std::cout << "Key found in ConfigSection" << std::endl;
			auto typed_value = std::dynamic_pointer_cast<TypedConfigValue<T>>(it->second);
			if (typed_value) {
				std::cout << "Successfully cast to TypedConfigValue<" << typeid(T).name() << ">" << std::endl;
				return typed_value->getValue();
			} else {
				std::cout << "Failed to cast to TypedConfigValue<" << typeid(T).name() << ">" << std::endl;
			}
		} else {
			std::cout << "Key not found in ConfigSection" << std::endl;
		}
		throw std::runtime_error("Key not found or type mismatch: " + key);
	}
	
	// Specialization for std::string to avoid unnecessary conversion
	template<>
	std::string ConfigSection::getValue<std::string>(const std::string& key) const {
		std::cout << "ConfigSection::getValue<std::string> called for key: " << key << std::endl;
		auto it = values.find(key);
		if (it != values.end()) {
			auto string_value = std::dynamic_pointer_cast<TypedConfigValue<std::string>>(it->second);
			if (string_value) {
				return string_value->getValue();
			}
		}
		throw std::runtime_error("Key not found: " + key);
	}
	
	// Specialization for std::vector<double>
	template<>
	std::vector<double> ConfigSection::getValue<std::vector<double>>(const std::string& key) const {
		std::cout << "ConfigSection::getValue<std::vector<double>> called for key: " << key << std::endl;
		auto it = values.find(key);
		if (it != values.end()) {
			auto string_value = std::dynamic_pointer_cast<TypedConfigValue<std::string>>(it->second);
			if (string_value) {
				std::vector<double> result;
				std::istringstream iss(string_value->getValue());
				std::string token;
				while (std::getline(iss, token, ',')) {
					result.push_back(std::stod(token));
				}
				return result;
			}
		}
		throw std::runtime_error("Key not found or invalid format: " + key);
	}
	
	bool ConfigSection::hasKey(const std::string& key) const {
		return values.find(key) != values.end();
	}
	
	void ConfigSection::setValidationRule(const std::string& key, const ValidationRules::Rule* rule) {
        validationRules[key] = rule;
    }
	
	const std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& ConfigSection::getValues() const {
		return values;
	}
	
	ConfigReader::ConfigReader() : filepath("") {
		std::cout << "ConfigReader constructor started" << std::endl;
		//initialize();
		std::cout << "ConfigReader constructor finished" << std::endl;
	}
	
	void ConfigReader::initialize() {
		std::cout << "ConfigReader::initialize started" << std::endl;
		try {
			std::cout << "Calling getConfigFilePath()" << std::endl;
			filepath = getConfigFilePath();
			std::cout << "Config file path: " << filepath << std::endl;
	
			// Check if the file exists, if not, generate it
			std::ifstream file(filepath);
			if (!file.is_open()) {
				std::cout << "Config file not found. Generating new file." << std::endl;
				generateConfigFile(*this);
			}
			file.close();
	
			std::cout << "Calling loadConfig()" << std::endl;
			loadConfig();
			std::cout << "Config loaded" << std::endl;
			
			std::cout << "Setting validation rules" << std::endl;
			setValidationRules();
			std::cout << "Validation rules set" << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "Exception in ConfigReader::initialize: " << e.what() << std::endl;
			throw;
		} catch (...) {
			std::cerr << "Unknown exception in ConfigReader::initialize" << std::endl;
			throw;
		}
		std::cout << "ConfigReader::initialize finished" << std::endl;
	}
	
	
	template<typename T>
	T ConfigReader::getValue(const std::string& section, const std::string& key) const {
		std::cout << "Attempting to get value for section: " << section << ", key: " << key << std::endl;
		auto sect_it = sections.find(section);
		if (sect_it != sections.end()) {
			std::cout << "Section found" << std::endl;
			return sect_it->second.getValue<T>(key);
		}
		std::cout << "Section not found" << std::endl;
		throw std::runtime_error("Section not found: " + section);
	}
	
	template<typename T>
	void ConfigReader::setValue(const std::string& section, const std::string& key, const T& value) {
		sections[section].setValue(key, value);
	}
	
	void ConfigReader::setValue(const std::string& section, const std::string& key, const std::string& value) {
		sections[section].setValue(key, value);
	}
	
	void ConfigReader::setValue(const std::string& section, const std::string& key, const std::vector<double>& value) {
		sections[section].setValue(key, value);
	}
	
	template<>
	void ConfigReader::setValue<std::string>(const std::string& section, const std::string& key, const std::string& value) {
		auto& sectionObj = sections[section];
		auto it = sectionObj.getValues().find(key);
		if (it != sectionObj.getValues().end()) {
			it->second->fromString(value);
		} else {
			sectionObj.setValue(key, value);
		}
	}
	
	bool ConfigReader::hasValue(const std::string& section, const std::string& key) const {
		auto sect_it = sections.find(section);
		if (sect_it != sections.end()) {
			return sect_it->second.hasKey(key);
		}
		return false;
	}
	
	
	void ConfigReader::setValidationRule(const std::string& section, const std::string& key, const ValidationRules::Rule* rule) {
        sections[section].setValidationRule(key, rule);
    }
	
	void ConfigReader::setValidationRules() {
        auto configSections = getConfigSections();
        for (const auto& section : configSections) {
            for (const auto& item : section.items) {
                if (item.validationRule) {
                    setValidationRule(section.name, item.name, item.validationRule);
                }
            }
        }
    }
	
void ConfigReader::loadConfig() {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filepath << std::endl;
        return;
    }

    std::string current_section;
    std::string line;
    while (std::getline(file, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[' && line.back() == ']') {
			current_section = line.substr(1, line.size() - 2);
		} else {
            auto pos = line.find('=');
			if (pos != std::string::npos) {
				std::string key = trim(line.substr(0, pos));
				std::string value = line.substr(pos + 1);
				
				// Remove comments from the value
				size_t commentPos = value.find('#');
				if (commentPos != std::string::npos) {
					value = value.substr(0, commentPos);
				}
				value = trim(value);
				
                if (!current_section.empty()) {
                    auto configSections = getConfigSections();
                    for (const auto& section : configSections) {
                        if (section.name == current_section) {
                            for (const auto& item : section.items) {
                                if (item.name == key) {
                                    try {
                                        if (std::string(item.type) == "double") {
                                            double doubleValue = std::stod(value);
                                            if (!item.validationRule || (*item.validationRule)(TypedConfigValue<double>(doubleValue))) {
                                                setValue(current_section, key, doubleValue);
                                            } else {
                                                std::cerr << "Validation failed for " << current_section << "." << key 
                                                          << ". Using default value." << std::endl;
                                                useDefaultValue(current_section, key, item);
                                            }
                                        } else if (std::string(item.type) == "int") {
                                            int intValue = std::stoi(value);
                                            if (!item.validationRule || (*item.validationRule)(TypedConfigValue<int>(intValue))) {
                                                setValue(current_section, key, intValue);
                                            } else {
                                                std::cerr << "Validation failed for " << current_section << "." << key 
                                                          << ". Using default value." << std::endl;
                                                useDefaultValue(current_section, key, item);
                                            }
                                        } else if (std::string(item.type) == "vector<double>") {
											std::vector<double> vec;
											// remove any comments from the value string
											std::string cleanValue = value.substr(0, value.find('#'));
											cleanValue = trim(cleanValue);  // Trim any trailing spaces after removing comment
										
											std::istringstream iss(cleanValue);
											std::string token;
											bool parseError = false;
											while (std::getline(iss, token, ',')) {
												token = trim(token);
												if (token.empty()) continue; // Skip empty elements
												try {
													size_t pos;
													double num = std::stod(token, &pos);
													if (pos != token.length()) {
														throw std::invalid_argument("Invalid characters in number");
													}
													vec.push_back(num);
												} catch (const std::exception& e) {
													std::cerr << "Error parsing vector element '" << token << "': " << e.what() << std::endl;
													parseError = true;
													break;
												}
											}
											if (!parseError && (!item.validationRule || (*item.validationRule)(TypedConfigValue<std::vector<double>>(vec)))) {
												setValue(current_section, key, vec);
											} else {
												std::cerr << "Validation failed or parse error for " << current_section << "." << key 
														<< ". Using default value." << std::endl;
												useDefaultValue(current_section, key, item);
											}
										} else {
                                            if (!item.validationRule || (*item.validationRule)(TypedConfigValue<std::string>(value))) {
                                                setValue(current_section, key, value);
                                            } else {
                                                std::cerr << "Validation failed for " << current_section << "." << key 
                                                          << ". Using default value." << std::endl;
                                                useDefaultValue(current_section, key, item);
                                            }
                                        }
                                    } catch (const std::exception& e) {
                                        std::cerr << "Error processing " << current_section << "." << key 
                                                  << ": " << e.what() << ". Using default value." << std::endl;
                                        useDefaultValue(current_section, key, item);
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}
	
	void ConfigReader::setValueWithValidation(const std::string& section, const std::string& key, const std::string& value) {
		auto configSections = getConfigSections();
		for (const auto& configSection : configSections) {
			if (configSection.name == section) {
				for (const auto& item : configSection.items) {
					if (item.name == key) {
						if (std::string(item.type) == "double") {
							double doubleValue = std::stod(value);
							setValue(section, key, doubleValue);
						} else if (std::string(item.type) == "int") {
							int intValue = std::stoi(value);
							setValue(section, key, intValue);
						} else if (std::string(item.type) == "vector<double>") {
							std::vector<double> vec;
							std::istringstream iss(value);
							std::string token;
							while (std::getline(iss, token, ',')) {
								vec.push_back(std::stod(token));
							}
							setValue(section, key, vec);
						} else {
							setValue(section, key, value);
						}
						return;
					}
				}
			}
		}
		throw std::runtime_error("Key not found in configuration");
	}
	
	void ConfigReader::useDefaultValue(const std::string& section, const std::string& key, const ConfigGen::ConfigItem& item) {
		setValueWithValidation(section, key, item.defaultValue);
	}
	
	void ConfigReader::saveConfig() const {
		std::ofstream file(filepath);
		if (!file.is_open()) {
			throw std::runtime_error("Unable to open file for writing: " + filepath);
		}
	
		for (const auto& section : sections) {
			file << "[" << section.first << "]\n";
			for (const auto& value : section.second.getValues()) {
				file << value.first << " = " << value.second->toString() << "\n";
			}
			file << "\n";
		}
	}
	
	std::string ConfigReader::trim(const std::string& str) {
		const auto strBegin = str.find_first_not_of(" \t");
		if (strBegin == std::string::npos) return "";
		const auto strEnd = str.find_last_not_of(" \t");
		const auto strRange = strEnd - strBegin + 1;
		return str.substr(strBegin, strRange);
	}
	
	void generateConfigFile(const ConfigReader& reader) {
		std::string filePath = reader.getConfigFilePath();
		std::ifstream file(filePath);
		
		// TODO (IHT): Update to boost::filesystem::exists(filePath)
		if (file.is_open()) {
			std::cout << "Configuration file already exists. Skipping generation." << std::endl;
			return;
		}
		
		auto sections = reader.getConfigSections();
	
		// Perform runtime validation
		if (!ConfigGen::validateConfig(sections)) {
			throw std::runtime_error("Invalid configuration detected at runtime");
		}
		
		// Generate the configuration content
		std::string configContent = ConfigGen::generateConfig(sections);
	
		std::ofstream configFile(filePath);
		if (configFile.is_open()) {
			configFile << configContent;
			configFile.close();
		} else {
			throw std::runtime_error("Unable to open file for writing: " + filePath);
		}
	}
	
	// Compile-time check
	static_assert(ConfigGen::validateConfigStructure(), "Invalid configuration structure detected at compile-time");
	
	// Explicit template instantiations
	template class TypedConfigValue<int>;
	template class TypedConfigValue<double>; 
	template class TypedConfigValue<std::string>;
	template class TypedConfigValue<std::vector<double>>;
	
	template void ConfigSection::setValue<int>(const std::string&, const int&);
	template void ConfigSection::setValue<double>(const std::string&, const double&);
	template void ConfigSection::setValue<std::string>(const std::string&, const std::string&);
	template void ConfigSection::setValue<std::vector<double>>(const std::string&, const std::vector<double>&);

	
	template int ConfigSection::getValue<int>(const std::string&) const;
	template double ConfigSection::getValue<double>(const std::string&) const;
	template std::string ConfigSection::getValue<std::string>(const std::string&) const;
	template std::vector<double> ConfigSection::getValue<std::vector<double>>(const std::string&) const;
	
	template int ConfigReader::getValue<int>(const std::string&, const std::string&) const;
	template double ConfigReader::getValue<double>(const std::string&, const std::string&) const;
	template std::string ConfigReader::getValue<std::string>(const std::string&, const std::string&) const;
	template std::vector<double> ConfigReader::getValue<std::vector<double>>(const std::string&, const std::string&) const;
	
	template void ConfigReader::setValue<int>(const std::string&, const std::string&, const int&);
	template void ConfigReader::setValue<double>(const std::string&, const std::string&, const double&);
	template void ConfigReader::setValue<std::string>(const std::string&, const std::string&, const std::string&);
	template void ConfigReader::setValue<std::vector<double>>(const std::string&, const std::string&, const std::vector<double>&);
	
} // namespace ConfigLib