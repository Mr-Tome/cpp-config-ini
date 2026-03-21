#include <iostream>
#include "config_library/config_reader/config_reader.hpp"
#include "config_library/common/validation_rules.hpp"

class SimpleCLIConfig : public ConfigLib::ConfigReader<SimpleCLIConfig, ConfigLib::CLI>
{
public:
	SimpleCLIConfig(int argc, char* argv[]): ConfigLib::ConfigReader<SimpleCLIConfig, ConfigLib::CLI>(argc, argv)
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
				"Run",
				{
					Item::make<int>("verbosity", 1, "Logging verbosity level (0=silent, 3=debug)", nullptr),
                    Item::make<int>("num_threads", 4, "Number of worker threads", &ValidationRules::greaterThanZero),
                    Item::make<double>("timeout_s", 30.0, "Per-task timeout in seconds", &ValidationRules::greaterThanZero),
                    Item::make<bool>("dry_run", false, "If true, no output is written to disk", nullptr),
                    Item::make<std::string>("run_id", std::string(""), "Unique identifier for this run", nullptr, P::Volatile)
				}
			}
		};
	}
};

struct SimpleCLI
{
	SimpleCLIConfig config;
	SimpleCLI(int argc, char* argv[]) : config(argc, argv)
	{
		run_simple_cli_config();
	}
	void run_simple_cli_config()
	{
		std::cout << "\n--- SimpleCLIConfig (CLI-only) ---" << std::endl;
		try
		{
			std::cout << "verbosity:   " << config.getValue<int>   ("Run", "verbosity")   << std::endl;
			std::cout << "num_threads: " << config.getValue<int>   ("Run", "num_threads") << std::endl;
			std::cout << "timeout_s:   " << config.getValue<double>("Run", "timeout_s")   << std::endl;
			std::cout << "dry_run:     " << config.getValue<bool>  ("Run", "dry_run")     << std::endl;
			std::cout << "run_id:      '" << config.getValue<std::string>("Run", "run_id") << "'" << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "SimpleCLIConfig error: " << e.what() << std::endl;
		}
		std::cout << "--- SimpleCLIConfig done ---\n" << std::endl;
	}
};
	

