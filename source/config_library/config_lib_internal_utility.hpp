#include <string>

namespace ConfigLib {
namespace Internal {

inline std::string trim(const std::string& str)
{
    const auto strBegin = str.find_first_not_of(" \t\r\n");
    if (strBegin == std::string::npos) return "";
    const auto strEnd   = str.find_last_not_of(" \t\r\n");
    return str.substr(strBegin, strEnd - strBegin + 1);
}

} // namespace Internal
} // namespace ConfigLib


