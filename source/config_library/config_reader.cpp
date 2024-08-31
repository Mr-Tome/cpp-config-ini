#include "config_reader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>

// Implementation of TypedConfigValue methods
template<typename T>
TypedConfigValue<T>::TypedConfigValue(const T& val) : value(val) {}

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


// Implementation of ConfigSection methods
template<typename T>
void ConfigSection::setValue(const std::string& key, const T& value) {
    std::cout << "ConfigSection::setValue called for key: " << key << std::endl;
    try {
        auto it = values.find(key);
        if (it != values.end()) {
            auto typed_value = std::dynamic_pointer_cast<TypedConfigValue<T>>(it->second);
            if (typed_value) {
                typed_value->setValue(value);
            } else {
                values[key] = std::make_shared<TypedConfigValue<T>>(value);
            }
        } else {
            values[key] = std::make_shared<TypedConfigValue<T>>(value);
        }
        
        if (validationRules.count(key) > 0) {
            if (!validationRules[key](*values[key])) {
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

template<typename T>
T ConfigSection::getValue(const std::string& key) const {
    auto it = values.find(key);
    if (it != values.end()) {
        auto typed_value = std::dynamic_pointer_cast<TypedConfigValue<T>>(it->second);
        if (typed_value) {
            return typed_value->getValue();
        }
    }
    throw std::runtime_error("Key not found or type mismatch: " + key);
}

bool ConfigSection::hasKey(const std::string& key) const {
    return values.find(key) != values.end();
}

void ConfigSection::setValidationRule(const std::string& key, ValidationRule rule) {
    validationRules[key] = rule;
}

const std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& ConfigSection::getValues() const {
    return values;
}

// Implementation of ConfigReader methods
ConfigReader::ConfigReader() : filepath("") {
    std::cout << "ConfigReader constructor started" << std::endl;
    std::cout << "ConfigReader constructor finished" << std::endl;
}
void ConfigReader::initialize() {
    std::cout << "ConfigReader::initialize started" << std::endl;
    try {
        std::cout << "Calling setDefaultValues()" << std::endl;
        setDefaultValues();
        std::cout << "Default values set" << std::endl;

        std::cout << "Calling getConfigFilePath()" << std::endl;
        filepath = getConfigFilePath();
        std::cout << "Config file path: " << filepath << std::endl;

        std::cout << "Calling loadConfig()" << std::endl;
        loadConfig();
        std::cout << "Config loaded" << std::endl;
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
    auto sect_it = sections.find(section);
    if (sect_it != sections.end()) {
        return sect_it->second.getValue<T>(key);
    }
    throw std::runtime_error("Section not found: " + section);
}

template<typename T>
void ConfigReader::setValue(const std::string& section, const std::string& key, const T& value) {
    sections[section].setValue(key, value);
}

bool ConfigReader::hasValue(const std::string& section, const std::string& key) const {
    auto sect_it = sections.find(section);
    if (sect_it != sections.end()) {
        return sect_it->second.hasKey(key);
    }
    return false;
}

template<typename T>
void ConfigReader::setDefaultValue(const std::string& section, const std::string& key, const T& value) {
    std::cout << "ConfigReader::setDefaultValue called for " << section << "." << key << std::endl;
    try {
        sections[section].setValue(key, value);
        std::cout << "Default value set for " << section << "." << key << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in ConfigReader::setDefaultValue: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "Unknown exception in ConfigReader::setDefaultValue" << std::endl;
        throw;
    }
}

void ConfigReader::setValidationRule(const std::string& section, const std::string& key, ValidationRule rule) {
    sections[section].setValidationRule(key, rule);
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
                std::string value = trim(line.substr(pos + 1));

                if (!current_section.empty()) {
                    if (sections[current_section].hasKey(key)) {
                        sections[current_section].getValues().at(key)->fromString(value);
                    } else {
                        sections[current_section].setValue(key, value);
                    }
                }
            }
        }
    }
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

template void ConfigReader::setDefaultValue<int>(const std::string&, const std::string&, const int&);
template void ConfigReader::setDefaultValue<double>(const std::string&, const std::string&, const double&);
template void ConfigReader::setDefaultValue<std::string>(const std::string&, const std::string&, const std::string&);
template void ConfigReader::setDefaultValue<std::vector<double>>(const std::string&, const std::string&, const std::vector<double>&);