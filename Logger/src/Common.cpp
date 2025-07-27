#include "Common.h"

namespace lgx {
    namespace utils {
        [[nodiscard]] auto DeserializeFmtColorType(const std::string_view serializedString) noexcept
            -> std::optional<fmt::detail::color_type>
        {
            fmt::detail::color_type type;

            if (serializedString.compare("{null}") == 0)
                return std::nullopt;

            const auto value_start = serializedString.find("value=") + sizeof("value=") - 1;
            if (value_start != std::string::npos)
            {
                const auto end = serializedString.substr(value_start).find("}");
                const auto str = serializedString.substr(value_start, end);
                std::from_chars(str.data(), str.data() + str.size(), type.value_);
            }
            return type;
        }

        [[nodiscard]] auto DeserializeFmtStyle(const std::string_view serializedString) noexcept -> fmt::text_style
        {
            fmt::text_style style;
            const auto fg_color_start = serializedString.find("foreground_color=") + sizeof("foreground_color=") - 1;
            if (fg_color_start != std::string::npos)
            {
                const auto      part = serializedString.substr(fg_color_start);
                std::size_t     end  = 0;
                std::list<char> braces;
                for (std::size_t i = 0; i < part.size(); ++i)
                {
                    if (part[i] == '{')
                        braces.push_back('{');
                    else if (part[i] == '}')
                    {
                        braces.pop_back();
                        if (braces.empty())
                        {
                            end = fg_color_start + i + 1;
                            break;
                        }
                    }
                }
                const auto colour =
                    DeserializeFmtColorType(serializedString.substr(fg_color_start, end - fg_color_start));
                if (colour)
                    style |= fmt::fg(*colour);
            }

            const auto bg_color_start = serializedString.find("background_color=") + sizeof("background_color=") - 1;
            if (bg_color_start != std::string::npos)
            {
                const auto      part = serializedString.substr(bg_color_start);
                std::size_t     end  = 0;
                std::list<char> braces;
                for (std::size_t i = 0; i < part.size(); ++i)
                {
                    if (part[i] == '{')
                        braces.push_back('{');
                    else if (part[i] == '}')
                    {
                        braces.pop_back();
                        if (braces.empty())
                        {
                            end = bg_color_start + i + 1;
                            break;
                        }
                    }
                }
                const auto colour =
                    DeserializeFmtColorType(serializedString.substr(bg_color_start, end - bg_color_start));
                if (colour)
                    style |= fmt::bg(*colour);
            }

            const auto emphasis_start = serializedString.find("emphasis=") + sizeof("emphasis=") - 1;
            if (emphasis_start != std::string::npos)
            {
                const auto   end = serializedString.substr(emphasis_start).find("}");
                const auto   str = serializedString.substr(emphasis_start, end);
                std::uint8_t emphasis;
                std::from_chars(str.data(), str.data() + str.size(), emphasis);
                style |= static_cast<fmt::emphasis>(emphasis);
            }

            return style;
        }
    } // namespace utils

    [[nodiscard]] auto LogMsg::FromString(const std::string_view serializedString) noexcept -> LogMsg
    {
        LogMsg msg;

        const auto msg_start = serializedString.find("message=") + sizeof("message=") - 1;
        if (msg_start != std::string::npos)
        {
            const auto end     = serializedString.substr(msg_start).find(";") + 1;
            const auto msg_str = serializedString.substr(msg_start, end);
            if (msg_str.compare("null;") != 0)
            {
                for (std::size_t i = 0; i < msg_str.size(); ++i)
                {
                    if (msg_str[i] == '\'' && msg_str[i + 1] == ';')
                    {
                        msg.message = msg_str.substr(1, i - 1);
                        break;
                    }
                }
            }
        }

        const auto prefix_start = serializedString.find("prefix=") + sizeof("prefix=") - 1;
        if (prefix_start != std::string::npos)
        {
            const auto end        = serializedString.substr(prefix_start).find(";") + 1;
            const auto prefix_str = serializedString.substr(prefix_start, end);
            if (prefix_str.compare("null;") != 0)
            {
                for (std::size_t i = 0; i < prefix_str.size(); ++i)
                {
                    if (prefix_str[i] == '\'' && prefix_str[i + 1] == ';')
                    {
                        msg.prefix = prefix_str.substr(1, i - 1);
                        break;
                    }
                }
            }
        }

        const auto level_start = serializedString.find("level=") + sizeof("level=") - 1;
        if (level_start != std::string::npos)
        {
            using enum Level;

            const auto end       = serializedString.substr(level_start).find(";");
            const auto level_str = serializedString.substr(level_start, end);
            if (level_str.compare("Info") == 0)
                msg.level = Info;
            else if (level_str.compare("Warning") == 0)
                msg.level = Warn;
            else if (level_str.compare("Error") == 0)
                msg.level = Error;
            else if (level_str.compare("Fatal") == 0)
                msg.level = Fatal;
        }

        const auto style_start = serializedString.find("defaultStyle=") + sizeof("defaultStyle=") - 1;
        if (style_start != std::string::npos)
        {
            std::list<char> braces;
            const auto      part = serializedString.substr(style_start);
            std::size_t     end  = 0;
            for (std::size_t i = 0; i < part.size(); ++i)
            {
                if (part[i] == '{')
                    braces.push_back('{');
                else if (part[i] == '}')
                {
                    braces.pop_back();
                    if (braces.empty())
                    {
                        end = style_start + i + 1;
                        break;
                    }
                }
            }
            msg.style = utils::DeserializeFmtStyle(serializedString.substr(style_start, end - style_start));
        }
        return msg;
    }

    [[nodiscard]] auto LogMsg::ToString(const LogMsg& log) noexcept -> std::string
    {
        return fmt::format(
            "{{message={};prefix={};level={};defaultStyle={}}}", (log.message.empty()) ? "null" : '\'' + log.message + '\'',
            (log.prefix) ? '\'' + *log.prefix + '\'' : "null", log.level, utils::SerializeFmtStyle(log.style));
    }
} // namespace lgx
