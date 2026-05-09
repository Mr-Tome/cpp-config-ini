#include "config_logger.hpp"

namespace ConfigLib {
    static LogCallback s_logCallback;

    void setLogger(LogCallback callback) {
        s_logCallback = std::move(callback);
    }

    namespace Internal {
        void log(std::string_view message) {
            if (s_logCallback) s_logCallback(message);
        }
    }
}
