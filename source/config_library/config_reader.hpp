#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include "validation_rules.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>


namespace ConfigGen {
    struct ConfigItem {
        const char* name;
        const char* type;
        const char* defaultValue;
        const char* description;
        const ValidationRules::Rule* validationRule;
    };

    struct ConfigSection {
        const char* name;
        const ConfigItem* items;
        size_t itemCount;
    };

    bool validateConfig(const ConfigSection* sections, size_t sectionCount);
    std::string generateConfig(const ConfigSection* sections, size_t sectionCount);
}

class ConfigValue {
public:
    virtual ~ConfigValue() = default;
    virtual std::string toString() const = 0;
    virtual void fromString(const std::string& str) = 0;
    virtual std::shared_ptr<ConfigValue> clone() const = 0;
};

template<typename T>
class TypedConfigValue : public ConfigValue {
public:
    TypedConfigValue(const T& val);
    const T& getValue() const;
    void setValue(const T& val);
    std::string toString() const override;
    void fromString(const std::string& str) override;
    std::shared_ptr<ConfigValue> clone() const override;

private:
    T value;
};

using ValidationRule = std::function<bool(const ConfigValue&)>;

class ConfigSection {
public:
    template<typename T>
    void setValue(const std::string& key, const T& value);

    template<typename T>
    T getValue(const std::string& key) const;

    bool hasKey(const std::string& key) const;
    void setValidationRule(const std::string& key, ValidationRule rule);
    const std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& getValues() const;

private:
    std::unordered_map<std::string, std::shared_ptr<ConfigValue>> values;
    std::unordered_map<std::string, ValidationRule> validationRules;
};

class ConfigReader {
public:
    ConfigReader();
    virtual ~ConfigReader() = default;

    void initialize();

    template<typename T>
    T getValue(const std::string& section, const std::string& key) const;

    template<typename T>
    void setValue(const std::string& section, const std::string& key, const T& value);

    bool hasValue(const std::string& section, const std::string& key) const;

    template<typename T>
    void setDefaultValue(const std::string& section, const std::string& key, const T& value);

    void setValidationRule(const std::string& section, const std::string& key, ValidationRule rule);
    void saveConfig() const;

    virtual std::string getConfigFilePath() const = 0;
    virtual const ConfigGen::ConfigSection* getConfigSections(size_t& sectionCount) const = 0;
	
	const std::unordered_map<std::string, ConfigSection>& getSections() const { return sections; }

protected:
    virtual void setDefaultValues() = 0;

    void loadConfig();
    static std::string trim(const std::string& str);

    std::string filepath;
    std::unordered_map<std::string, ConfigSection> sections;
};

void generateConfigFile(const ConfigReader& reader);

#endif // CONFIG_READER_H