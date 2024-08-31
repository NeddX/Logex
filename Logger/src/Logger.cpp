#include "Logger.h"

namespace lgx {
    namespace internal {
        Logger g_GlobalLogger = Logger{ Logger::Properties{ .outputStream = std::cout, .defaultPrefix = "Global" } };
        std::unordered_map<std::string, Logger> g_Loggers;
    } // namespace internal
} // namespace lgx
