#include "test_framework.hpp"
#include "config_library/common/config_logger.hpp"

using ConfigLib::setLogger;
using ConfigLib::Internal::log;

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

int main()
{
    return runTests({
        {"Logger: default (no callback) is silent",          test_default_logger_is_silent},
        {"Logger: set callback receives messages",           test_set_logger_receives_messages},
        {"Logger: replacing callback calls only the new one", test_replacing_logger_calls_only_second},
        {"Logger: setting nullptr silences output",          test_null_logger_silences_output},
    });
}
