#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>



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

protected:
    virtual void setDefaultValues() = 0;
    virtual std::string getConfigFilePath() const = 0;

    void loadConfig();
    static std::string trim(const std::string& str);

    std::string filepath;
    std::unordered_map<std::string, ConfigSection> sections;
};


#endif // CONFIG_READER_H