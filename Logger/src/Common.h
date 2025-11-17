#pragma once

#include <atomic>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <future>
#include <iostream>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include <fmt/args.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#ifdef __unix__
#include <syslog.h>
#endif

// TODO: Remove this macro and replace its instances with just inline.
#define LGX_CONSTEXPR inline

namespace lgx {
    enum class Level : std::uint8_t
    {
        Info = 0,
        Warn,
        Error,
        Fatal,
        Debug,
        Verbose
    };

    enum class Type : std::uint8_t
    {
        User,
        Daemon
    };

    // For shorthand.
    using enum Level;
} // namespace lgx

namespace fmt {
    template <>
    struct formatter<lgx::Level> : formatter<std::string_view>
    {
        [[nodiscard]] auto inline format(const lgx::Level& level, format_context& ctx) const noexcept
        {
            std::string_view name;
            switch (level)
            {
                using enum lgx::Level;

                case Info: name = "Info"; break;
                case Warn: name = "Warn"; break;
                case Error: name = "Error"; break;
                case Fatal: name = "Fatal"; break;
                case Debug: name = "Debug"; break;
                case Verbose: name = "Verbose"; break;
            };

            return formatter<std::string_view>::format(name, ctx);
        }
    };
} // namespace fmt

namespace lgx {
    namespace utils {
        [[nodiscard]] LGX_CONSTEXPR auto SerializeFmtColorType(const fmt::detail::color_type& type) noexcept
            -> std::string
        {
            return fmt::format("{{value={}}}", type.value());
        }
        [[nodiscard]] LGX_CONSTEXPR auto SerializeFmtStyle(const fmt::text_style& style) noexcept -> std::string
        {
            return fmt::format("{{foreground_color={};background_color={};emphasis={}}}",
                               style.has_foreground() ? SerializeFmtColorType(style.get_foreground()) : "{null}",
                               style.has_background() ? SerializeFmtColorType(style.get_background()) : "{null}",
                               (style.has_emphasis()) ? static_cast<std::uint8_t>(style.get_emphasis()) : 0);
        }
        [[nodiscard]] auto DeserializeFmtColorType(const std::string_view serializedString) noexcept
            -> std::optional<fmt::detail::color_type>;
        [[nodiscard]] auto DeserializeFmtStyle(const std::string_view serializedString) noexcept -> fmt::text_style;
    } // namespace utils

    struct LogMsg
    {
    public:
        Level                      level;
        std::string                message;
        std::optional<std::string> prefix = std::nullopt;
        fmt::text_style            style;

    public:
        [[nodiscard]] static auto FromString(const std::string_view serializedString) noexcept -> LogMsg;
        [[nodiscard]] static auto ToString(const LogMsg& log) noexcept -> std::string;
    };
} // namespace lgx
