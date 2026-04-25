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

	if (std::ifstream(confg_file).good())
		std::cout << "Found " << confg_file << " (created by ./run). Migrating...\n\n";
	else
	{
		std::cerr << "ERROR: " << confg_file << " not found.\n"
				  << "Run ./run first to create the file, then re-run this example.\n";
		return 1;
	}
	
    // ------------------------------------------------------------------
    // Test 2: Load with V2. Migration runs automatically on startup:
    //   - appends ",255" to every 3-component Color value in the file
    //   - file is rewritten with 4-component values + schema_version = 2
    // ------------------------------------------------------------------
	ColorConfigV2 cfg(argc, argv);

	std::cout << "\n--- Results after migration ---\n\n";

	const Color red   = cfg.getValue<Color>("FirstColor",        "red");
	const Color green = cfg.getValue<Color>("FirstColor",        "green");
	const Color blue  = cfg.getValue<Color>("FirstColor",        "blue");
	const Color black = cfg.getValue<Color>("secondColor_group", "black");
	const Color white = cfg.getValue<Color>("secondColor_group", "white");
	const Color gray  = cfg.getValue<Color>("secondColor_group", "gray");

	auto check = [](const std::string& label, const Color& c, int eb, int eg, int eb2, int ebr)
	{
		const bool pass = (c.red == eb && c.green == eg && c.blue == eb2 && c.brightness == ebr);
		std::cout << "  " << (pass ? "[PASS]" : "[FAIL]") << "  " << label
				  << " = " << c.toString() << "\n";
	};

	check("FirstColor.red  ", red,   255, 0,   0,   255);
	check("FirstColor.green", green, 0,   255, 0,   255);
	check("FirstColor.blue ", blue,  0,   0,   255, 255);
	check("black           ", black, 0,   0,   0,   255);
	check("white           ", white, 255, 255, 255, 255);
	check("gray            ", gray,  128, 128, 128, 255);

	// Show that brightness is now independently settable.
	std::cout << "\n  Setting gray to half-brightness (128,128,128,128)...\n";
	cfg.setValue("secondColor_group", "gray", Color(128, 128, 128, 128));
	cfg.saveConfig();

	const Color gray2 = cfg.getValue<Color>("secondColor_group", "gray");
	const bool pass = (gray2.brightness == 128);
	std::cout << "  " << (pass ? "[PASS]" : "[FAIL]")
			  << "  gray after set = " << gray2.toString() << "\n";

	std::cout << "\nInspect " << confg_file << " to see:\n"
			  << "  - # __schema_version__ = 2 at the top\n"
			  << "  - All Color values stored as r,g,b,brightness\n"
			  << "  - gray = 128,128,128,128\n"
			  << "\nRun ./run_ex_color_migration again — schema evolution should\n"
			  << "report no differences (file already at v2).\n";
    return 0;
}
