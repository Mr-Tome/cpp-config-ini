// ex_color_evolution
//
// Evolves color_config_v1.ini — the same file used by ./run — to add a brightness
// component to every stored Color value.
//
// The Color type gains a 4th field (brightness, default 255 = max).
// fromString accepts both the old "r,g,b" format and the new "r,g,b,brightness"
// format so the type works correctly before and after migration.
//
// Because Color keeps the same typeName() ("Color"), the schema evolver does NOT
// see a type change — the only thing that changes is the stored string format.
// Migration::transformInPlace appends ",255" to every 3-component value in the file.
//
// Intended workflow:
//   ./run                     -- creates / uses color_config_v1.ini with 3-component Colors
//   ./run_ex_color_migration  -- migrates the same file, adds brightness in-place
//   ./run_ex_color_migration  -- run again: no migration output, file already at v2

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "config_library/config_reader/config_reader.hpp"

static const std::string confg_file = "color_config_v1.ini";

struct Color : public ConfigLib::ConfigType<Color>
{
    int red, green, blue;
    int brightness; // new field — default 255 (max)

    Color() : red(0), green(0), blue(0), brightness(255) {}
    Color(int r, int g, int b, int br = 255)
        : red(r), green(g), blue(b), brightness(br) {}

    static const char* typeName() { return "Color"; }

    std::string toString() const
    {
        std::ostringstream o;
        o << red << "," << green << "," << blue << "," << brightness;
        return o.str();
    }

    // accepts both "r,g,b" (brightness defaults to 255) and "r,g,b,brightness".
    static Color fromString(const std::string& s)
    {
        Color c;
        std::istringstream iss(s);
        char comma;
        iss >> c.red >> comma >> c.green >> comma >> c.blue;
        if (iss >> comma >> c.brightness) {}
        else c.brightness = 255;
        return c;
    }
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
					{"red", Color(255,0,0), "this is the color red", nullptr},
					{"green", Color(0,255,0), "this is the color green", nullptr},
					{"blue", "Color", "0,0,255", "this is the color blue.", nullptr},
					{"sample_bool", "bool", "true", "this is true.", nullptr}
				}
			},
			{ 
				"secondColor_group",
				{
					{"black", "Color", "0,0,0", "this is the color red", nullptr},
					{"white", "Color", "255,255,255", "this is the color green", nullptr},
					{"gray", "Color", "128,128,128", "this is the color blue.", nullptr},
					{"sample_vector_int", "vector<int>", "0,1,2,2,3,4,5", "this is an int vector.", nullptr},
					{"sample_vector_string", "vector<string>", "0,1asdf,2,2,3213f,4,5", "this is a string vector.", nullptr}
				}
			}
		};
	}
};

// -----------------------------------------------------------------------
// V2 config — Color now stores brightness. Schema version bumped to 2.
// getConfigSections() is identical to V1 (same type name, same defaults)
// except defaults now include brightness = 255 via Color(r,g,b,255).
//
// getMigrations() appends ",255" to every 3-component stored value.
// The lambda is idempotent: 4-component values (3 commas) are left unchanged.
// -----------------------------------------------------------------------
class ColorConfigV2
    : public ConfigLib::ConfigReader<ColorConfigV2, ConfigLib::INI, ConfigLib::CLI>
{
public:
    ColorConfigV2(int argc, char* argv[])
        : ConfigLib::ConfigReader<ColorConfigV2, ConfigLib::INI, ConfigLib::CLI>(argc, argv) {}

    std::string getConfigFilePath() const { return confg_file; }

    uint32_t getSchemaVersion() const { return 2u; }

    std::vector<ConfigLib::SchemaMigration> getMigrations() const
    {
        // Append ",255" to any 3-component Color value (2 commas).
        // Idempotent: 4-component values (3 commas) pass through unchanged.
        auto addBrightness = [](const std::string& val) -> std::string
        {
            const int commas = static_cast<int>(
                std::count(val.begin(), val.end(), ','));
            return (commas == 2) ? val + ",255" : val;
        };

        return {
            ConfigLib::Migration::transformInPlace(1u, 2u, "FirstColor",       "red",   addBrightness),
            ConfigLib::Migration::transformInPlace(1u, 2u, "FirstColor",       "green", addBrightness),
            ConfigLib::Migration::transformInPlace(1u, 2u, "FirstColor",       "blue",  addBrightness),
            ConfigLib::Migration::transformInPlace(1u, 2u, "secondColor_group","black", addBrightness),
            ConfigLib::Migration::transformInPlace(1u, 2u, "secondColor_group","white", addBrightness),
            ConfigLib::Migration::transformInPlace(1u, 2u, "secondColor_group","gray",  addBrightness),
        };
    }

    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {
            {
                "FirstColor",
				{
					{"red", Color(255,0,0), "this is the color red", nullptr},
					{"green", Color(0,255,0), "this is the color green", nullptr},
					{"blue", "Color", "0,0,255", "this is the color blue.", nullptr},
					{"sample_bool", "bool", "true", "this is true.", nullptr}
				}
            },
            {
                "secondColor_group",
				{
					{"black", "Color", "0,0,0", "this is the color black", nullptr},
					{"white", "Color", "255,255,255", "this is the color white", nullptr},
					{"gray", "Color", "128,128,128", "this is the color gray.", nullptr},
					{"sample_vector_int", "vector<int>", "0,1,2,2,3,4,5", "this is an int vector.", nullptr},
					{"sample_vector_string", "vector<string>", "0,1asdf,2,2,3213f,4,5", "this is a string vector.", nullptr}
				}
            }
        };
    }
};

int main(int argc, char* argv[])
{
    std::cout << "=== ex_color_migration ===\n\n";

    // ------------------------------------------------------------------
    // Test 1: load the file as  and set a custom color so we can
    // confirm the value survives the migration.
    // ------------------------------------------------------------------
    std::cout << "--- storing a custom coral color ---\n\n";
    {
        ColorConfigIniClass cfg(argc, argv);

        const Color existing = cfg.getValue<Color>("FirstColor", "red");
        std::cout << "  Current FirstColor.red = " << existing.toString() << "\n"
                  << "  Setting to coral (255,127,80)...\n";

        cfg.setValue("FirstColor", "red", Color(255, 127, 80));
        cfg.saveConfig();
        
        auto assertCoralColor = cfg.getValue<Color>("FirstColor", "red");
        if(assertCoralColor.red != 255 ||
			assertCoralColor.green != 127 ||
			assertCoralColor.blue != 80 )
		{
			throw std::logic_error("Test 1: Assert Coral Color failed!");
		}
    }
	
    // ------------------------------------------------------------------
    // Test 2: Load with V2. Migration runs automatically on startup:
    //   - appends ",255" to every 3-component Color value in the file
    //   - file is rewritten with 4-component values + schema_version = 2
    // ------------------------------------------------------------------
    std::cout << "\n--- Test 2: V2 — brightness added via migration ---\n\n";
    {
        ColorConfigV2 cfg(argc, argv);

        const Color assertCoralColor = cfg.getValue<Color>("FirstColor","red");
        const Color green = cfg.getValue<Color>("FirstColor","green");
        const Color blue = cfg.getValue<Color>("FirstColor","blue");
        const Color black = cfg.getValue<Color>("secondColor_group","black");
        const Color white = cfg.getValue<Color>("secondColor_group","white");
        const Color gray = cfg.getValue<Color>("secondColor_group","gray");
		
		if(assertCoralColor.red != 255 ||
			assertCoralColor.green != 127 ||
			assertCoralColor.blue != 80 )
		{
			throw std::logic_error("Test 2: Assert Coral Color failed!");
		}
		
        std::cout << "  FirstColor.red   = " << assertCoralColor.toString()
                  << "  (expected 255,127,80,255 — coral carried forward, brightness added)\n"
                  << "  FirstColor.green = " << green.toString()
                  << "  (expected 0,255,0,255)\n"
                  << "  FirstColor.blue  = " << blue.toString()
                  << "  (expected 0,0,255,255)\n"
                  << "  black            = " << black.toString()
                  << "  (expected 0,0,0,255)\n"
                  << "  white            = " << white.toString()
                  << "  (expected 255,255,255,255)\n"
                  << "  gray             = " << gray.toString()
                  << "  (expected 128,128,128,255)\n";

        std::cout << "\n  Setting gray to half-brightness: (128,128,128,128)...\n";
        cfg.setValue("secondColor_group", "gray", Color(128, 128, 128, 128));
        cfg.saveConfig();
    }

    std::cout << "\n Currently, inspect " << confg_file << " to see:\n"
              << "  - # __schema_version__ = 2 at the top\n"
              << "  - All Color values now stored as r,g,b,brightness\n"
              << "  - gray = 128,128,128,128\n"
              << "\nRun ./run_ex_color_evolution again — schema evolution should\n"
              << "report no differences (file already at v2).\n";
    return 0;
}
