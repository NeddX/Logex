#include <fstream>
#include <iostream>

#include <Logger.h>

template <typename... TArgs>
struct A
{
};

auto main() -> int
{
    auto a = A();

    auto logger = logex::Logger(logex::Logger::Properties{

    });

    logger.Info("An Info where {} + {} is {}.", 5, 5, 5 + 5);
    logger.Warn("A Warning.");
    logger.Error("An Error.");
    logger.Fatal("A Fatal error has occured.");

    auto fs = std::ofstream("./log.txt", std::ios::out);
    auto file_logger =
        logex::Logger(logex::Logger::Properties{ .output = fs, .format = "[{datetime}] [{type}] >> {msg}\n" });

    file_logger.Info("An Info in a file.");
    file_logger.Warn("A Warning in a file.");
    file_logger.Error("An Error in a file.");
    file_logger.Fatal("A fatal error in a file.");
}