#include <fstream>
#include <iostream>

#include <Logger.h>

auto main() -> int
{
    // Create logger instances.
    const auto logger = lgx::Logger(lgx::Logger::Properties{ .defaultPrefix = "main.cpp" });

    logger.Info("Current file: {}", __FILE__);
    logger.Warn("A Warning.");
    logger.Error("Error code: {}", std::rand() % 256);
    logger.Fatal("A Fatal error has occured.");

    // Log to different streams e.g., std::ofstream, std::fstream, std::stringstream, std::ostringstream etc...
    auto       fs = std::ofstream("./log.txt");
    const auto file_logger =
        lgx::Logger{ lgx::Logger::Properties{ .outputStreams               = { &fs },
                                              .serializeToNonStdoutStreams = true,
                                              .defaultPrefix               = "log.txt",
                                              .defaultStyle = { .format = "[{datetime}] [{level}] ({prefix}) >> {msg}\n" } } };

    file_logger.Info("Current file: {}", __FILE__);
    file_logger.Warn("A Warning.");
    file_logger.Error("Error code: {}", std::rand() % 256);
    file_logger.Fatal("A Fatal error has occured.");

    // Log using the global logger.
    lgx::Log(lgx::Info, "The global logger");
    lgx::Log("App", lgx::Warn, fmt::fg(fmt::color::orange) | fmt::bg(fmt::color::dark_blue),
             "Fancy customization using fmt.");

    // Serializable log messages.
    const auto serialized_log = lgx::LogMsg::ToString(
        lgx::LogMsg{ .level   = lgx::Error,
                     .message = fmt::format("An error occured in file: {}", __FILE__),
                     .style   = fmt::fg(fmt::color::black) | fmt::emphasis::bold | fmt::bg(fmt::color::aqua) });

    // Create a logger instance inside the global registry.
    lgx::New("debug_logger", lgx::Logger::Properties{ .defaultPrefix = "DebugLogger" });
    lgx::Get("debug_logger").Log(lgx::Info, "Debug logger");

    // Customize the global logger.
    // You can also customize logger instances as well by calling the same method
    // e.g., file_logger.SetDefaultInfoStyle(fmt::fg(fmt::color::dark_green));
    lgx::SetDefaultInfoStyle(fmt::fg(fmt::color::dark_green));
    lgx::SetDefaultWarnStyle(fmt::fg(fmt::color::yellow));
    lgx::SetDefaultErrorStyle(fmt::fg(fmt::color::red));
    lgx::SetDefaultFatalStyle(fmt::fg(fmt::color::dark_red));
    lgx::SetDefaultPrefix("App");

    lgx::Log(lgx::Info, "Serialized (to string) log message: {}", serialized_log);
    lgx::LogMsg deserialized_log = lgx::LogMsg::FromString(serialized_log);
    lgx::Log(deserialized_log);
}
