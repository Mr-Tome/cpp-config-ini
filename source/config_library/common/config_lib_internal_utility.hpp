#include <string>
#include <string_view>

namespace ConfigLib {
namespace Internal {

[[nodiscard]] inline std::string trim(std::string_view str)
{
    const auto strBegin = str.find_first_not_of(" \t\r\n");
    if (strBegin == std::string_view::npos) return "";
    const auto strEnd   = str.find_last_not_of(" \t\r\n");
    return std::string(str.substr(strBegin, strEnd - strBegin + 1));
}

} // namespace Internal
} // namespace ConfigLib


