#include "Logger.h"

namespace lgx {
    namespace internal {
        Logger g_GlobalLogger = Logger{ Logger::Properties{ .outputStream = std::cout, .defaultPrefix = "Global" } };
    } // namespace internal
} // namespace logex