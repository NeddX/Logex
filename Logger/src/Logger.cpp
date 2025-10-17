#include "Logger.h"

namespace lgx {
    auto Get(const std::string& loggerName) -> Logger&
    {
        static std::unordered_map<std::string, Logger> loggers = { { "global", lgx::Logger::Properties{
                                                                                   .defaultPrefix = "Global",
                                                                               } } };
        static std::shared_mutex                       global_registry_mutex;

        // Soft lock first in case the logger already exists.
        {
            std::shared_lock<std::shared_mutex> lock{ global_registry_mutex };
            auto                                it = loggers.find(loggerName);
            if (it != loggers.end())
                return it->second;
        }

        std::unique_lock<std::shared_mutex> lock{ global_registry_mutex };
        auto                                it = loggers.find(loggerName);
        if (it == loggers.end())
        {
            it = loggers.emplace(loggerName, Logger{ { .defaultPrefix = loggerName } }).first;
        }
        return it->second;
    }
} // namespace lgx
