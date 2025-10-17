#include "Logger.h"

namespace lgx {
    auto Get(const std::string& loggerName) -> Logger& {
        static std::unordered_map<std::string, Logger> loggers = {
        { "global", lgx::Logger::Properties {
            .defaultPrefix = "Global",
        }}
    };
        return loggers[loggerName];
    }
} // namespace lgx
