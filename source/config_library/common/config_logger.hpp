#pragma once
#include <functional>
#include <string_view>

namespace ConfigLib {
    using LogCallback = std::function<void(std::string_view)>;
    // Sets the global diagnostic log callback. Pass nullptr (or call with no
    // argument after setting) to silence output. Thread-unsafe — call once at
    // startup before using the library.
    void setLogger(LogCallback callback);

    namespace Internal {
        // Called by library internals. No-ops if no logger is set.
        void log(std::string_view message);
    }
}
