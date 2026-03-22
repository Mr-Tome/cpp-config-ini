#include <iostream>
#include "config_library/config_reader/config_reader.hpp"
#include "config_library/common/validation_rules.hpp"

class Simple_CLI_INIConfig : public ConfigLib::ConfigReader<Simple_CLI_INIConfig, ConfigLib::CLI, ConfigLib::INI>
{
public:
	Simple_CLI_INIConfig(int argc, char* argv[]): ConfigLib::ConfigReader<Simple_CLI_INIConfig, ConfigLib::CLI, ConfigLib::INI>(argc, argv)
	{
		std::cout<<"SimpleCLI constructor called" << std::endl;
	}
	
	std::vector<ConfigLib::ConfigSection> getConfigSections() const
	{
		using Item = ConfigLib::ConfigItem;
		using P    = ConfigLib::Persistence;
		
		return
		{
			{
				"section1",
				{
					Item::make<int>("verbosity", 1, "Logging verbosity level (0=silent, 3=debug)", nullptr),
                    Item::make<int>("num_threads", 4, "Number of worker threads", &ValidationRules::greaterThanZero),
                    Item::make<double>("timeout_s", 30.0, "Per-task timeout in seconds", &ValidationRules::greaterThanZero),
                    Item::make<bool>("dry_run", false, "If true, no output is written to disk", nullptr),
                    Item::make<std::string>("run_id", std::string(""), "Unique identifier for this run", nullptr, P::Volatile)
				}
			},
			{
				"section2",
				{
					Item::make<int>("verbosity2", 1, "Logging verbosity level (0=silent, 3=debug)", nullptr),
                    Item::make<int>("num_threads", 4, "Number of worker threads", &ValidationRules::greaterThanZero),
                    Item::make<double>("timeout_s", 30.0, "Per-task timeout in seconds", &ValidationRules::greaterThanZero),
                    Item::make<bool>("dry_run2", false, "If true, no output is written to disk", nullptr),
                    Item::make<std::string>("run_id", std::string(""), "Unique identifier for this run", nullptr, P::Volatile)
				}
			},
			{
				"",
				{
					Item::make<int>("trial", 1, "Logging verbosity level (0=silent, 3=debug)", nullptr),
					Item::make<std::string>("flat", std::string(""), "attempt to cause bug", nullptr, P::Volatile),
					Item::make<bool>("help", true, "attempt to cause bug2", nullptr),
				}
			},
			{
				"",
				{
					Item::make<int>("trial2", 1, "Logging verbosity level (0=silent, 3=debug)", nullptr)
				}
			}
		};
	}
	
	std::string getConfigFilePath() const
	{
		return "simple_cli_ini_config.ini";
	}
};

struct Simple_CLI_INI
{
	Simple_CLI_INIConfig config;
	Simple_CLI_INI(int argc, char* argv[]) : config(argc, argv)
	{
		run_simple_cli_config();
	}
	void run_simple_cli_config()
	{
		std::cout << "\n--- Simple_CLI_INIConfig (CLI-INI-only) ---" << std::endl;
		try
		{
			std::cout << "verbosity:   " << config.getValue<std::string>   ("", "flat")   << std::endl;
			std::cout << "num_threads: " << config.getValue<bool>   ("", "help") << std::endl;
			std::cout << "timeout_s:   " << config.getValue<double>("section2", "timeout_s")   << std::endl;
			std::cout << "dry_run:     " << config.getValue<bool>  ("section1", "dry_run")     << std::endl;
			std::cout << "run_id:      '" << config.getValue<std::string>("section1", "run_id") << "'" << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Simple_CLI_INIConfig error: " << e.what() << std::endl;
		}
		std::cout << "--- Simple_CLI_INIConfig done ---\n" << std::endl;
	}
};
	

