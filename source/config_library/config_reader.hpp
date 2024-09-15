#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include "validation_rules.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

namespace ConfigLib {
   namespace ConfigGen {
       struct ConfigItem {
           const char* name;
           const char* type;
           const char* defaultValue;
           const char* description;
           const ValidationRules::Rule* validationRule;
       };

       struct ConfigSection {
           std::string name;
           std::vector<ConfigItem> items;
       };

       bool validateConfig(const std::vector<ConfigSection>& sections);
       std::string generateConfig(const std::vector<ConfigSection>& sections);
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

   //using ValidationRule = std::function<bool(const ConfigValue&)>;

class ConfigSection {
public:
    template<typename T>
    void setValue(const std::string& key, const T& value);

    void setValue(const std::string& key, const std::string& value);
    void setValue(const std::string& key, const std::vector<double>& value);

    template<typename T>
    T getValue(const std::string& key) const;

    bool hasKey(const std::string& key) const;
    void setValidationRule(const std::string& key, const ValidationRules::Rule* rule);
    const std::unordered_map<std::string, std::shared_ptr<ConfigValue>>& getValues() const;

private:
    std::unordered_map<std::string, std::shared_ptr<ConfigValue>> values;
    std::unordered_map<std::string, const ValidationRules::Rule*> validationRules;
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

    void setValue(const std::string& section, const std::string& key, const std::string& value);
    void setValue(const std::string& section, const std::string& key, const std::vector<double>& value);

    bool hasValue(const std::string& section, const std::string& key) const;

    void setValidationRule(const std::string& section, const std::string& key, const ValidationRules::Rule* rule);
    void saveConfig() const;

    virtual std::string getConfigFilePath() const = 0;
    virtual std::vector<ConfigGen::ConfigSection> getConfigSections() const = 0;
    
    const std::unordered_map<std::string, ConfigSection>& getSections() const { return sections; }

protected:
    void loadConfig();
    static std::string trim(const std::string& str);
    void setValidationRules();
    
    std::string filepath;
    std::unordered_map<std::string, ConfigSection> sections;
	
private:
    void useDefaultValue(const std::string& section, const std::string& key, const ConfigGen::ConfigItem& item);
    void setValueWithValidation(const std::string& section, const std::string& key, const std::string& value);
	
};

void generateConfigFile(const ConfigReader& reader);
} // namespace ConfigLib

#endif // CONFIG_READER_H