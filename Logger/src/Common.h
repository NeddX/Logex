#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <charconv>

#include <fmt/args.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>

// TODO: Remove this macro and replace its instances with just inline.
#define LGX_CONSTEXPR inline

namespace lgx {
    enum class Level : std::uint8_t
    {
        Info = 0,
        Warn,
        Error,
        Fatal
    };
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
                case Warn: name = "Warning"; break;
                case Error: name = "Error"; break;
                case Fatal: name = "Fatal"; break;
            };

            return formatter<std::string_view>::format(name, ctx);
        }
    };
} // namespace fmt

namespace lgx {
    namespace utils {
        [[nodiscard]] LGX_CONSTEXPR auto SerializeFmtColorType(
            const fmt::detail::color_type& type) noexcept -> std::string
        {
            return fmt::format("{{is_rgb={};value={}}}", type.is_rgb, type.value.rgb_color);
        }
        [[nodiscard]] LGX_CONSTEXPR auto SerializeFmtStyle(const fmt::text_style& style) noexcept -> std::string
        {
            return fmt::format("{{foreground_color={};background_color={};emphasis={}}}",
                               style.has_foreground() ? SerializeFmtColorType(style.get_foreground()) : "{null}",
                               style.has_background() ? SerializeFmtColorType(style.get_background()) : "{null}",
                               (style.has_emphasis()) ? static_cast<std::uint8_t>(style.get_emphasis()) : 0);
        }
        [[nodiscard]] auto DeserializeFmtColorType(
            const std::string_view serializedString) noexcept -> std::optional<fmt::detail::color_type>;
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
