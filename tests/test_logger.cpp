#include "test_framework.hpp"
#include "config_library/common/config_logger.hpp"
#include "config_library/config_reader/config_reader.hpp"

using ConfigLib::setLogger;
using ConfigLib::Internal::log;

// ─── Minimal CLI config for integration tests ─────────────────────────────────

struct LoggerIntegrationConfig
    : public ConfigLib::ConfigReader<LoggerIntegrationConfig, ConfigLib::CLI>
{
    std::vector<ConfigLib::ConfigSection> getConfigSections() const
    {
        return {{"S", {ConfigLib::ConfigItem::make<int>("x", 42, "x value")}}};
    }
};

static bool test_default_logger_is_silent()
{
    setLogger({});  // ensure no callback is set
    std::string captured;
    log("should not appear");
    REQUIRE(captured.empty());
    return true;
}

static bool test_set_logger_receives_messages()
{
    std::string captured;
    setLogger([&captured](std::string_view msg) { captured += msg; });
    log("hello logger");
    setLogger({});
    REQUIRE(!captured.empty());
    REQUIRE_EQ(captured, std::string("hello logger"));
    return true;
}

static bool test_replacing_logger_calls_only_second()
{
    std::string first, second;
    setLogger([&first](std::string_view msg) { first += msg; });
    setLogger([&second](std::string_view msg) { second += msg; });
    log("replacement message");
    setLogger({});
    REQUIRE(first.empty());
    REQUIRE(!second.empty());
    return true;
}

static bool test_null_logger_silences_output()
{
    std::string captured;
    setLogger([&captured](std::string_view msg) { captured += msg; });
    setLogger({});  // silence
    log("this should be dropped");
    REQUIRE(captured.empty());
    return true;
}

// ─── Integration: library internals route through the logger ─────────────────

static bool test_logger_captures_getValue_messages()
{
    std::string captured;
    {
        SuppressStdout suppress;
        LoggerIntegrationConfig cfg;
        setLogger([&captured](std::string_view msg) { captured += msg; });
        (void)cfg.getValue<int>("S", "x");
        setLogger({});
    }
    REQUIRE(captured.find("Attempting to get value") != std::string::npos);
    return true;
}

static bool test_logger_captures_section_not_found_message()
{
    std::string captured;
    {
        SuppressStdout suppress;
        LoggerIntegrationConfig cfg;
        setLogger([&captured](std::string_view msg) { captured += msg; });
        try { (void)cfg.getValue<int>("Missing", "x"); } catch (...) {}
        setLogger({});
    }
    REQUIRE(captured.find("Section not found") != std::string::npos);
    return true;
}

static bool test_logger_captures_setValue_messages()
{
    std::string captured;
    {
        SuppressStdout suppress;
        LoggerIntegrationConfig cfg;
        setLogger([&captured](std::string_view msg) { captured += msg; });
        cfg.setValue<int>("S", "x", 99);
        setLogger({});
    }
    REQUIRE(captured.find("setValue") != std::string::npos);
    return true;
}

int main()
{
    return runTests({
        {"Logger: default (no callback) is silent",               test_default_logger_is_silent},
        {"Logger: set callback receives messages",                test_set_logger_receives_messages},
        {"Logger: replacing callback calls only the new one",     test_replacing_logger_calls_only_second},
        {"Logger: setting nullptr silences output",               test_null_logger_silences_output},
        {"Logger: getValue routes through logger",                test_logger_captures_getValue_messages},
        {"Logger: missing section routes through logger",         test_logger_captures_section_not_found_message},
        {"Logger: setValue routes through logger",                test_logger_captures_setValue_messages},
    });
}
