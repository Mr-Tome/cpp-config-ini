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
	   
		class ConfigItemBase {
		public:
			virtual ~ConfigItemBase() = default;
			virtual std::string getName() const = 0;
			virtual std::string getType() const = 0;
			virtual std::string getDefaultValueString() const = 0;
			virtual std::string getDescription() const = 0;
			virtual const ValidationRules::Rule* getValidationRule() const = 0;
		};
		
		template<typename T>
		class ConfigItem : public ConfigItemBase {
		public: 
           const std::string name;
           const char* type;
           const T defaultValue;
           const std::string description;
           const ValidationRules::Rule* validationRule;
		   
		   ConfigItem(
				std::string n,
				T defVal,
				std::string desc,
				const ValidationRules::Rule* rule = nullptr)
				: name(std::move(n)),
				defaultValue(std::move(defVal)),
				description(std::move(desc)),
				validationRule(rule) {}
				
			const std::string getName() const override { return name; }
			const std::string getDescription() const override { return description; }
			const ValidationRules::Rule* getValidationRule() const override { return validationRule; }
			
			std::string getType() const override {
				if (std::is_same<T, int>::value) return "int";
				if (std::is_same<T, double>::value) return "double";
				if (std::is_same<T, std::string>::value) return "string";
				if (std::is_same<T, std::vector<double>>::value) return "vector<double>";
				return "unknown type";
			}
			
			std::string getDefaultValueString() const override {
				std::ostringstream oss;
				oss << defaultValue;
				return oss.str();
			}
		};
		
		// Specialization for std::vector<double>
		template<>
		inline std::string ConfigItem<std::vector<double>>::getDefaultValueString() const {
			std::ostringstream oss;
			for (size_t i = 0; i < defaultValue.size(); ++i) {
				if (i > 0) oss << ",";
				oss << defaultValue[i];
			}
			return oss.str();
		}
	
        struct ConfigSection {
            std::string name;
			std::vector<std::unique_ptr<ConfigItemBase>> items;
			
			template<typename... Args>
			ConfigSection(std::string n, Args&&... args) : name(std::move(n)) {
				(addItem(std::forward<Args>(args)), ...);
			}
		private:
			template<typename T, typename... NextItem>
			void addItem(const std::string& name, T&& defaultValue, const std::string& description, const ValidationRules::Rule* rule, NextItem&&... nextItem) {
				items.push_back(std::make_unique<ConfigItem<std::decay_t<T>>>(name, std::forward<T>(defaultValue), description, rule));
				if constexpr (sizeof...(nextItem) > 0) {
					addItem(std::forward<NextItem>(nextItem)...);
				}
			}
		
			template<typename T, typename... NextItem>
			void addItem(const std::string& name, T&& defaultValue, const std::string& description, NextItem&&... nextItem) {
				items.push_back(std::make_unique<ConfigItem<std::decay_t<T>>>(name, std::forward<T>(defaultValue), description, nullptr));
				if constexpr (sizeof...(nextItem) > 0) {
					addItem(std::forward<NextItem>(nextItem)...);
				}
			}
		};

       bool validateConfig(const std::vector<ConfigSection>& sections);
       std::string generateConfig(const std::vector<ConfigSection>& sections);
   }

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
	
		void saveConfig() const;
	
		virtual std::string getConfigFilePath() const = 0;
		virtual std::vector<ConfigGen::ConfigSection> getConfigSections() const = 0;
		
		const std::unordered_map<std::string,std::unordered_map<std::string,std::unique_ptr<ConfigGen::ConfigItemBase>>>& getSections() const { return sections; }
	
	protected:
		void loadConfig();
		static std::string trim(const std::string& str);
		
		std::string filepath;
		std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<ConfigGen::ConfigItemBase>>> sections;
		
	private:
		void useDefaultValue(const std::string& section, const std::string& key, const ConfigGen::ConfigItemBase& item);
		void setValueWithValidation(const std::string& section, const std::string& key, const std::string& value);
		
	};
	
	void generateConfigFile(const ConfigReader& reader);
} // namespace ConfigLib

#endif // CONFIG_READER_H