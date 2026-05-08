#pragma once
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <unordered_set>
#include "config_reader_base.hpp"
#include "PersistenceReaders/IPersistenceReader.hpp"
#include "../common/schema_evolver.hpp"
#include "../common/schema_migration.hpp"

namespace ConfigLib
{
namespace Internal
{

const std::string& iniInstructions();

void loadConfigFromFile(
	const std::string& filePath,
	const std::vector<ConfigSection>& configSections,
	std::unordered_map<std::string, ConfigSectionStore>& sections);

bool parseRawINI(
	const std::string& filePath,
	RawConfigMap& rawConfig);

std::string formatINI(
	const std::vector<ConfigSection>& configSections,
	std::function<std::pair<std::string, std::string>(
		const ConfigSection&, const ConfigItem&)> valueSource,
	std::function<std::string(const std::string& sectionName)> postSectionLines,
	const std::string& trailingContent,
	const std::string& header = "");

std::string evolveINI(
	const std::vector<ConfigSection>& currentSchema,
	const RawConfigMap& rawConfig,
	const SchemaEvolutionResult& result,
	OrphanedConfigItemPolicy policy,
	const std::string& header = "");

std::string iniDeprecatedKeyLines(
	const std::string& sectionName,
	const RawConfigMap& rawConfig,
	const std::unordered_set<std::string>& schemaKeySet,
	OrphanedConfigItemPolicy policy);

std::string iniOrphanedSections(
	const RawConfigMap& rawConfig,
	const std::unordered_set<std::string>& schemaSectionSet,
	OrphanedConfigItemPolicy policy);

std::pair<std::unordered_set<std::string>, std::unordered_set<std::string>>
buildIniSchemaSets(const std::vector<ConfigSection>& configSections);

} // namespace Internal


template<typename Derived, bool HasCLI = false>
class INIConfigReader : public ConfigReaderBase, public IPersistenceReader
{
	void init(const Derived& d,
			  const std::string& configFilePath)
	{
		auto mergedSections = mergeDuplicateSections(d.getConfigSections());

		if(!HasCLI)
			assertNoVolatileFieldsInINIOnlyReader(mergedSections);

		this->filepath = configFilePath.empty() ? d.getConfigFilePath() : configFilePath;
		initialize(mergedSections);
		generateConfigFileIfNeeded(this->filepath, mergedSections);
		runSchemaEvolution(d.getOrphanedConfigItemPolicy(), mergedSections);
		Internal::loadConfigFromFile(this->filepath, mergedSections, this->sections);
	}
public:
	INIConfigReader()
	{
		const Derived& d = static_cast<Derived&>(*this);
		init(d, d.getConfigFilePath());
	}

	explicit INIConfigReader(const std::string& configFilePathOverride)
	{
		init(static_cast<Derived&>(*this), configFilePathOverride);
	}

	OrphanedConfigItemPolicy getOrphanedConfigItemPolicy() const
	{
		return OrphanedConfigItemPolicy::CommentOut;
	}

	uint32_t getSchemaVersion() const
	{
		std::cout << "ini_config_reader.hpp: getSchemaVersion() " << std::endl;
		return Migration::invalidSchemaVersion;
	}
	std::vector<SchemaMigration> getMigrations() const { return {}; }

	void saveConfig() const
	{
		const Derived& d = static_cast<const Derived&>(*this);
		auto mergedSections = mergeDuplicateSections(d.getConfigSections());
		writeToFile(mergedSections, this->filepath);
	}

	void persistSave(const std::vector<ConfigSection>& configSections) const override
	{
		writeToFile(configSections, this->filepath);
	}

	void persistDelete() override
	{
		if (std::remove(this->filepath.c_str()) != 0)
		{
			throw std::runtime_error(
				"--delete: failed to delete config file: " + this->filepath
				+ ". Does it exist?");
		}
		std::cout << "--delete: deleted '" << this->filepath
		          << "'. Defaults will regenerate on the next run." << std::endl;
	}

	void persistExport(
		const std::string& exportPath,
		const std::vector<ConfigSection>& configSections) const override
	{
		writeToFile(configSections, exportPath);
		std::cout << "--export: wrote config to '" << exportPath << "'." << std::endl;
	}

	void persistSchemaDryRun(const std::vector<ConfigSection>& mergedSections) const override
	{
		const Derived& d = static_cast<const Derived&>(*this);

		RawConfigMap rawConfig;
		Internal::parseRawINI(this->filepath, rawConfig);

		const uint32_t schemaVersion = d.getSchemaVersion();
		if (schemaVersion > Migration::invalidSchemaVersion)
		{
			const uint32_t fileVersion = Migration::parseSchemaVersion(this->filepath);
			std::cout << "[--schema-dry-run] Current Schema version: " << schemaVersion << "\n"
			          << "File's Schema version: " << fileVersion << "\n";

			auto migrationPair = Migration::applyMigrations(
				std::move(rawConfig), d.getMigrations(), fileVersion, schemaVersion);
			rawConfig = std::move(migrationPair.first);
			Migration::logMigrationResult(migrationPair.second, this->filepath);
		}

		const auto result = evolveFileWithSchema(
			rawConfig, mergedSections, d.getOrphanedConfigItemPolicy());

		std::cout << "[--schema-dry-run] No file will be written.\n";
		logEvolutionResult(result, this->filepath);
		std::exit(0);
	}

	void persistSchemaVersion() const override
	{
		const Derived& d = static_cast<const Derived&>(*this);
		const uint32_t schemaVersion = d.getSchemaVersion();
		const uint32_t fileVersion   = Migration::parseSchemaVersion(this->filepath);

		std::cout << "Current Schema version: " << schemaVersion << "\n";
		std::cout << "File's Schema version: ";
		if (fileVersion == Migration::invalidSchemaVersion)
			std::cout << "(none file predates versioning)\n";
		else
			std::cout << fileVersion << "\n";

		std::exit(0);
	}

private:
	RawConfigMap postEvolutionRawConfig;

void runSchemaEvolution(
	OrphanedConfigItemPolicy policy,
	const std::vector<ConfigSection>& mergedSections)
{
	std::cout << "Running Schema Evolution" << std::endl;
	const Derived& d = static_cast<const Derived&>(*this);
	const uint32_t schemaVersion = d.getSchemaVersion();
	std::cout << "Current Schema Version: " << schemaVersion << std::endl;

	RawConfigMap rawConfig;
	Internal::parseRawINI(this->filepath, rawConfig);

	std::string versionHeader;
	bool versionHeaderChanged = false;

	if(schemaVersion > Migration::invalidSchemaVersion)
	{
		const uint32_t fileVersion = Migration::parseSchemaVersion(this->filepath);

		std::cout << "File's Schema Version: " << fileVersion << std::endl;

		versionHeaderChanged = (fileVersion != schemaVersion);

		auto migrationPair = Migration::applyMigrations(
			std::move(rawConfig), d.getMigrations(), fileVersion, schemaVersion);
		rawConfig = std::move(migrationPair.first);
		Migration::logMigrationResult(migrationPair.second, this->filepath);

		versionHeader = std::string(Migration::schemaVersionPrefix)
					  + std::to_string(schemaVersion);
	}

	const auto result = evolveFileWithSchema(rawConfig, mergedSections, policy);

	postEvolutionRawConfig = rawConfig;

	if(!result.fileModified && !versionHeaderChanged)
	{
		std::cout << "Schema Evolution found no differences!" << std::endl;
		return;
	}

	const std::string evolvedContent = Internal::evolveINI(mergedSections,
			rawConfig, result, policy, versionHeader);

	std::ofstream out(this->filepath);
	if (!out.is_open())
		throw std::runtime_error(
			"Schema evolution: unable to write evolved config to: " + this->filepath);

	out << evolvedContent;

	logEvolutionResult(result, this->filepath);

	std::cout << "Finished running Schema Evolution!" << std::endl;
}

void generateConfigFileIfNeeded(
	const std::string& filePath,
	const std::vector<ConfigSection>& mergedSections)
{
	std::ifstream file(filePath);

	if (file.is_open()) {
		std::cout << "Configuration file already exists. Skipping generation." << std::endl;
		return;
	}

	if (!validateConfig(mergedSections))
	{
		throw std::runtime_error("Invalid configuration detected at runtime");
	}

	std::string configContent = Internal::formatINI(
		mergedSections,
		[](const ConfigSection&, const ConfigItem& item)
			-> std::pair<std::string, std::string>
		{
			return {item.defaultValue, ""};
		},
		[](const std::string&) { return ""; },
		"");

	std::ofstream configFile(filePath);
	if (!configFile.is_open())
		throw std::runtime_error("Unable to open file for writing: " + filePath);

	configFile << configContent;
}

void writeToFile(
		const std::vector<ConfigSection>& configSections,
		const std::string& path) const
{
	std::cout << "Saving the current configuration to: " << path << std::endl;

	const Derived& d = static_cast<const Derived&>(*this);
	const uint32_t schemaVersion = d.getSchemaVersion();
	const std::string versionHeader = (schemaVersion > Migration::invalidSchemaVersion)
		? std::string(Migration::schemaVersionPrefix) + std::to_string(schemaVersion)
		: std::string();

	const auto schemaSets        = Internal::buildIniSchemaSets(configSections);
	const auto& schemaKeySet     = schemaSets.first;
	const auto& schemaSectionSet = schemaSets.second;

	RawConfigMap storedValues;
	for (const auto& section : configSections)
		for (const auto& item : section.items)
			if (item.persistence != Persistence::Volatile)
				storedValues[section.name][item.name] = this->sections.at(section.name).getValues().at(item.name)->toString();

	const OrphanedConfigItemPolicy policy = d.getOrphanedConfigItemPolicy();

	std::string content = Internal::formatINI(
		configSections,
		[&storedValues](const ConfigSection& section, const ConfigItem& item)
			-> std::pair<std::string, std::string>
		{
			return {storedValues.at(section.name).at(item.name), ""};
		},
		[&](const std::string& sectionName) -> std::string
		{
			return Internal::iniDeprecatedKeyLines(
				sectionName, postEvolutionRawConfig, schemaKeySet, policy);
		},
		Internal::iniOrphanedSections(postEvolutionRawConfig, schemaSectionSet, policy),
		versionHeader);

	std::ofstream out(path);
	if (!out.is_open())
		throw std::runtime_error("Unable to open file for writing: " + path);
	out << content;

	std::cout << "Finished saving configuration to: " << path << std::endl;
}

};

} // namespace ConfigLib
