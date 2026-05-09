#include <iostream>
#include <fstream>
#include <memory>
#include "config_library/config_reader/config_reader.hpp"
#include "MonteCarloConfig.hpp"
#include "SimpleCLI.hpp"
#include "Simple_CLI_INI.hpp"

class SpecificAlgorithmConfig : public ConfigLib::ConfigReader<SpecificAlgorithmConfig, ConfigLib::INI>
{
public:
    SpecificAlgorithmConfig() 
    {
        std::cout << "SpecificAlgorithmConfig called" << std::endl;
    }

    std::string getConfigFilePath() const 
    {
        std::cout << "Getting config file path" << std::endl;
        return "specific_algorithm_config.ini";
    }

    std::vector<ConfigLib::ConfigSection> getConfigSections() const 
    {
        static const ValidationRules::BetweenValues between0And100(0, 100);
        return {
            {
                "ABT",
                {
                    {"kor", -10.0, "ABT kor value", &ValidationRules::greaterThanZero},
                    ConfigLib::ConfigItem::make<int>("koh", 1, "ABT koh value", nullptr)
                }
            },
            {
                "TBM",
                {
                    ConfigLib::ConfigItem::make<double>("kor", 500.0, "TBM kor value", &ValidationRules::greaterThanZero)
                }
            },
            {
                "General",
                {
                    ConfigLib::ConfigItem::make<double>("FW", -999.0, "Fixed Wing value", &between0And100),
                    ConfigLib::ConfigItem::make<double>("RW", 20.0, "Rotary Wing value", &between0And100),
                    ConfigLib::ConfigItem::make<double>("CM", 1010.0, "Cruise Missile value", &between0And100),
                    ConfigLib::ConfigItem::make<std::vector<double>>("Misc", std::vector<double>{1.0, 2.0, 3.0}, "Misc item just for proof of principle", nullptr),
                    ConfigLib::ConfigItem::make<std::string>("string_test", std::string("ALL"), "string test", nullptr),
                    ConfigLib::ConfigItem::make<std::string>("std_string_test", std::string("wew"), "std::string test", nullptr)
                }
            }
        };
    }
};

void do_specific_algorithm_config()
{
	try {
        std::cout << "Creating SpecificAlgorithmConfig" << std::endl;
        SpecificAlgorithmConfig config;
        //config.initialize();
		
        // Print out the loaded configuration
        std::cout << "Loaded Configuration:" << std::endl;
        for (const auto& section : config.getSections()) {
            std::cout << "[" << section.first << "]" << std::endl;
            for (const auto& item : section.second.getValues()) {
                std::cout << item.first << " = " << item.second->toString() << std::endl;
            }
            std::cout << std::endl;
        }
        
		 // Use the config object like this
        try {
            std::cout << "1ABT.kor: " << config.getValue<double>("ABT", "kor") << std::endl;
			std::cout << "1ABT.koh: " << config.getValue<int>("ABT", "koh") << std::endl;
            //std::cout << "1TBM.kor: " << config.getValue<double>("TBM", "kor") << std::endl;
            //std::cout << "1General.FW: " << config.getValue<double>("General", "FW") << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error retrieving value: " << e.what() << std::endl;
        }
		
		// Use the config object like this
		double kor_new = config.getValue<double>("ABT", "kor") * 3.0;
		std::cout << "kor_new: " << std::to_string(kor_new) << std::endl;
		
        // Testing validation logic
        try {
            config.setValue("General", "FW", 100.0);
        } catch (const std::exception& e) {
            std::cerr << "Validation error: " << e.what() << std::endl;
        }

        // Use the config object like this
        try {
			
            std::cout << "2ABT.kor: " << config.getValue<double>("ABT", "kor") << std::endl;
			
            std::cout << "2ABT.kor: " << config.getValue<double>("ABT", "kor") << std::endl;
            std::cout << "2TBM.kor: " << config.getValue<double>("TBM", "kor") << std::endl;
            std::cout << "2General.FW: " << config.getValue<double>("General", "FW") << std::endl;
            
            std::cout << "2General.FW: " << config.getValue<std::string>("General", "string_test") << std::endl;
            std::cout << "2General.FW: " << config.getValue<std::string>("General", "std_string_test") << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error retrieving value: " << e.what() << std::endl;
        }
        
        auto misc_vec = config.getValue<std::vector<double>>("General","Misc");
        std::string string_misc;
        for(const auto& it:misc_vec)
        {
			if(!string_misc.empty())
			{
				string_misc+=", ";
			}
			string_misc += std::to_string(it);
		}
		std::cout << "Misc values: " << string_misc << std::endl;
        config.saveConfig();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

struct Color : public ConfigLib::ConfigType<Color>
{
	int red, green, blue;
	Color() : red(0), green(0),blue(0){}
	Color(int r, int g, int b) : red(r), green(g), blue(b){}
	
	
	static const char* typeName() {return "Color";}
	
	std::string toString() const
	{
		std::ostringstream output;
		output << red << "," << green << "," << blue;
		return output.str();
	}
	
	static Color fromString(const std::string& str)
	{
		Color output;
		std::istringstream iss(str);
		char comma; //doesnt matter what it is, just need 1 char...
		iss >> output.red >> comma >>output.green >> comma >> output.blue;
		return output;
	}
	
	//all 3 methods, so we dont get compile time errors...
};

struct ColorConfigIniClass : public ConfigLib::ConfigReader<ColorConfigIniClass, ConfigLib::INI, ConfigLib::CLI>
{
	ColorConfigIniClass(int argc, char* argv[]) : ConfigLib::ConfigReader<ColorConfigIniClass, ConfigLib::INI, ConfigLib::CLI>(argc, argv)
	{
        std::cout << "ColorConfigIniClass called" << std::endl;
	}
	
    std::string getConfigFilePath() const  
    {
        return "color_config_v1.ini";
    }
    
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
		return {
			{
				"FirstColor",
				{
					ConfigLib::ConfigItem::make<Color>("red", Color(255,0,0), "this is the color red", nullptr),
					ConfigLib::ConfigItem::make<Color>("green", Color(0,255,0), "this is the color green", nullptr),
					ConfigLib::ConfigItem::make<Color>("blue", Color(0,0,255), "this is the color blue.", nullptr),
					ConfigLib::ConfigItem::make<bool>("sample_bool", true, "this is true.", nullptr)
				}
			},
			{
				"secondColor_group",
				{
					ConfigLib::ConfigItem::make<Color>("black", Color(0,0,0), "this is the color red", nullptr),
					ConfigLib::ConfigItem::make<Color>("white", Color(255,255,255), "this is the color green", nullptr),
					ConfigLib::ConfigItem::make<Color>("gray", Color(128,128,128), "this is the color blue.", nullptr),
					ConfigLib::ConfigItem::make<std::vector<int>>("sample_vector_int", std::vector<int>{0,1,2,2,3,4,5}, "this is an int vector.", nullptr),
					ConfigLib::ConfigItem::make<std::vector<std::string>>("sample_vector_string", std::vector<std::string>{"0","1asdf","2","2","3213f","4","5"}, "this is a string vector.", nullptr)
				}
			}
		};
	}
};

void test_colors(int argc, char* argv[])
{
	ColorConfigIniClass my_color_config(argc, argv);
	
	/////testing the getter for Color's  ConfigType
	Color red =  my_color_config.getValue<Color>("FirstColor","red");
	
	std::cout << "red\'s red: " << red.red << std::endl;
	
	std::string red_color_string = red.toString();
	
	std::cout << "1Red Color String (r, g, b): " <<red_color_string << std::endl;
	std::cout << "2Red Color String (r, g, b): " <<red.toString() << std::endl;
	std::cout <<"////////////////////////////////////////////////////"<<std::endl;
	
	/////testing the setter for Color's  ConfigType (first setting, then getting...)
	Color black = Color();
	std::cout << "Black Color String (r, g, b): " <<black.toString() << std::endl;
	my_color_config.setValue<Color>("FirstColor","red", black);
	Color should_be_black = my_color_config.getValue<Color>("FirstColor","red");
	std::cout << "Should be Black Color String (r, g, b): " <<should_be_black.toString() << std::endl;
	
	my_color_config.saveConfig();
}

void do_monte_carlo()
{
	std::cout << "Monte Carlo Simulation started" << std::endl;
    
    try {
        MonteCarloSimulation simulation;
		
        double result = simulation.runSimulation();

        std::cout << "Simulation completed. Final result: " << result << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Program started" << std::endl;
    
    //do_specific_algorithm_config();
    test_colors(argc, argv);
    
    //do_monte_carlo();
    
    //auto test = SimpleCLI(argc, argv);
    //auto test2 = Simple_CLI_INI(argc, argv);
	
    std::cout << "Program finished" << std::endl;
    std::cout.flush(); 
    
    return 0;
}
