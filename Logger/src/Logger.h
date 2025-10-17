#include "Common.h"

namespace lgx {
    class Logger
    {
    public:
        struct DefaultStyle
        {
            std::string     format           = "[{datetime}] [{level}] ({prefix}): {msg}\n";
            fmt::text_style defaultInfoStyle = fmt::bg(fmt::color::dark_green) | fmt::fg(fmt::color::white);
            fmt::text_style defaultWarnStyle = fmt::bg(fmt::color::orange) | fmt::fg(fmt::color::black);
            fmt::text_style defaultErrorStyle =
                fmt::emphasis::italic | fmt::bg(fmt::color::red) | fmt::fg(fmt::color::white);
            fmt::text_style defaultFatalStyle =
                fmt::emphasis::italic | fmt::bg(fmt::color::dark_red) | fmt::fg(fmt::color::white);
        };
        struct Properties
        {
            std::vector<std::ostream*> outputStreams               = { &std::cout };
            bool                       unicodeSymbols              = false;
            bool                       serializeToNonStdoutStreams = false;
            std::string                defaultPrefix               = "App";
            std::string                dateTimeFormat              = "%Y-%m-%d %H:%M:%S";
            DefaultStyle               defaultStyle                = DefaultStyle{};
        };

    private:
        Properties         m_Properties;
        mutable std::mutex m_Guard;

    public:
        [[nodiscard]] inline auto GetDefaultPrefix() const noexcept -> std::string
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            return m_Properties.defaultPrefix;
        }
        [[nodiscard]] inline auto GetDateTimeFormat() const noexcept -> std::string
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            return m_Properties.dateTimeFormat;
        }
        [[nodiscard]] inline auto GetFormat() const noexcept -> std::string
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            return m_Properties.defaultStyle.format;
        }
        [[nodiscard]] inline auto GetDefaultInfoStyle() const noexcept -> fmt::text_style
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            return m_Properties.defaultStyle.defaultInfoStyle;
        }
        [[nodiscard]] inline auto GetDefaultWarnStyle() const noexcept -> fmt::text_style
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            return m_Properties.defaultStyle.defaultWarnStyle;
        }
        [[nodiscard]] inline auto GetDefaultErrorStyle() const noexcept -> fmt::text_style
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            return m_Properties.defaultStyle.defaultErrorStyle;
        }
        [[nodiscard]] inline auto GetDefaultFatalStyle() const noexcept -> fmt::text_style
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            return m_Properties.defaultStyle.defaultFatalStyle;
        }
        inline void SetDefaultPrefix(const std::string_view newDefaultPrefix) noexcept
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultPrefix = newDefaultPrefix;
        }
        inline void SetDateTimeFormat(const std::string_view newDateTimeFormat) noexcept
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.dateTimeFormat = newDateTimeFormat;
        }
        inline void SetFormat(const std::string_view newFormat) noexcept
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.format = newFormat;
        }
        inline void SetDefaultInfoStyle(const fmt::text_style& newDefaultInfoStyle) noexcept
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.defaultInfoStyle = newDefaultInfoStyle;
        }
        inline void SetDefaultWarnStyle(const fmt::text_style& newDefaultWarnStyle) noexcept
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.defaultWarnStyle = newDefaultWarnStyle;
        }
        inline void SetDefaultErrorStyle(const fmt::text_style& newDefaultErrorStyle) noexcept
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.defaultErrorStyle = newDefaultErrorStyle;
        }
        inline void SetDefaultFatalStyle(const fmt::text_style& newDefaultFatalStyle) noexcept
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.defaultFatalStyle = newDefaultFatalStyle;
        }

    public:
        Logger() noexcept {}
        Logger(Properties properties) noexcept
            : m_Properties(std::move(properties))
        {
        }
        Logger(const Logger& other) noexcept
            : m_Properties(other.m_Properties)
            , m_Guard()
        {
        }
        Logger(Logger&& other) noexcept
            : m_Properties(std::move(other.m_Properties))
            , m_Guard()
        {
        }
        ~Logger() noexcept {}

    private:
        [[nodiscard]] constexpr auto DefaultStyleFromLevel(const Level level) const noexcept -> fmt::text_style
        {
            switch (level)
            {
                using enum Level;

                case Info: return m_Properties.defaultStyle.defaultInfoStyle;
                case Warn: return m_Properties.defaultStyle.defaultWarnStyle;
                case Error: return m_Properties.defaultStyle.defaultErrorStyle;
                case Fatal: return m_Properties.defaultStyle.defaultFatalStyle;
            }
        }

    private:
        [[nodiscard]] static constexpr auto ContainsPlaceholder(const std::string_view format,
                                                                const std::string_view placeholder) noexcept -> bool
        {
            return format.find(placeholder) != std::string_view::npos;
        }

    public:
        void Log(const LogMsg& log) const
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };

            // Prepare arguments conditionally based on the presence of placeholders in the format string
            auto time_now = std::chrono::system_clock::now();
            auto time_obj = std::chrono::system_clock::to_time_t(time_now);

            auto arg_store = fmt::dynamic_format_arg_store<fmt::format_context>{};

            if (ContainsPlaceholder(m_Properties.defaultStyle.format, "{datetime}"))
            {
                arg_store.push_back(
                    fmt::arg("datetime", fmt::format(fmt::runtime("{:" + m_Properties.dateTimeFormat + '}'),
                                                     fmt::localtime(time_obj))));
            }
            if (ContainsPlaceholder(m_Properties.defaultStyle.format, "{level}"))
                arg_store.push_back(fmt::arg("level", log.level));
            if (ContainsPlaceholder(m_Properties.defaultStyle.format, "{prefix}"))
                arg_store.push_back(fmt::arg("prefix", log.prefix.value_or(m_Properties.defaultPrefix)));
            if (!ContainsPlaceholder(m_Properties.defaultStyle.format, "{msg}"))
                throw std::invalid_argument("A message is always required.");

            // Always push the message argument
            arg_store.push_back(fmt::arg("msg", log.message));

            for (const auto& stream : m_Properties.outputStreams)
            {
                if (stream == &std::cout)
                    fmt::vprint(stdout, log.style, m_Properties.defaultStyle.format, arg_store);
                else
                {
                    if (m_Properties.serializeToNonStdoutStreams)
                        *stream << LogMsg::ToString(log) << '\n';
                    else
                        *stream << fmt::vformat(m_Properties.defaultStyle.format, arg_store);
                }
            }
        }
        template <typename... TArgs>
        void Log(const std::string_view prefix, const Level level, const fmt::text_style& style,
                 const std::string_view fmt, TArgs&&... args) const
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };

            // Prepare arguments conditionally based on the presence of placeholders in the format string
            auto time_now = std::chrono::system_clock::now();
            auto time_obj = std::chrono::system_clock::to_time_t(time_now);

            auto arg_store = fmt::dynamic_format_arg_store<fmt::format_context>{};

            if (ContainsPlaceholder(m_Properties.defaultStyle.format, "{datetime}"))
            {
                arg_store.push_back(
                    fmt::arg("datetime", fmt::format(fmt::runtime("{:" + m_Properties.dateTimeFormat + '}'),
                                                     fmt::localtime(time_obj))));
            }
            if (ContainsPlaceholder(m_Properties.defaultStyle.format, "{level}"))
                arg_store.push_back(fmt::arg("level", level));
            if (ContainsPlaceholder(m_Properties.defaultStyle.format, "{prefix}"))
                arg_store.push_back(fmt::arg("prefix", prefix));
            if (!ContainsPlaceholder(m_Properties.defaultStyle.format, "{msg}"))
                throw std::invalid_argument("A message is always required.");

            // Always push the message argument
            arg_store.push_back(fmt::arg("msg", fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...)));

            for (const auto& stream : m_Properties.outputStreams)
            {
                if (stream == &std::cout)
                    fmt::vprint(stdout, style, m_Properties.defaultStyle.format, arg_store);
                else
                {
                    if (m_Properties.serializeToNonStdoutStreams)
                    {
                        const auto log = LogMsg{ .level   = level,
                                                 .message = fmt::vformat(m_Properties.defaultStyle.format, arg_store),
                                                 .prefix  = std::string{ prefix },
                                                 .style   = style };
                        *stream << LogMsg::ToString(log) << '\n';
                    }
                    else
                        *stream << fmt::vformat(m_Properties.defaultStyle.format, arg_store);
                }
            }
        }
        template <typename... TArgs>
        void Log(LogMsg log, const std::string_view fmt, TArgs&&... args) const
        {
            Log(log.prefix.value_or(m_Properties.defaultPrefix), log.level, log.style, fmt,
                std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void Log(const Level level, const std::string_view fmt, TArgs&&... args) const
        {
            Log(m_Properties.defaultPrefix, level, DefaultStyleFromLevel(level), fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void Log(const std::string_view prefix, const Level level, const std::string_view fmt, TArgs&&... args) const
        {
            Log(prefix, level, DefaultStyleFromLevel(level), fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void Log(const Level level, const fmt::text_style& style, const std::string_view fmt, TArgs&&... args) const
        {
            Log(m_Properties.defaultPrefix, level, style, fmt, std::forward<TArgs>(args)...);
        }

    public:
        template <typename... TArgs>
        void Info(const std::string_view fmt, TArgs&&... args) const
        {
            Log(Level::Info, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void Warn(const std::string_view fmt, TArgs&&... args) const
        {
            Log(Level::Warn, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void Error(const std::string_view fmt, TArgs&&... args) const
        {
            Log(Level::Error, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        void Fatal(const std::string_view fmt, TArgs&&... args) const
        {
            Log(Level::Fatal, fmt, std::forward<TArgs>(args)...);
        }
    };

    [[nodiscard]] auto Get(const std::string& loggerName) -> Logger&;

    [[nodiscard]] inline auto GetDefaultPrefix() noexcept
    {
        return Get("global").GetDefaultPrefix();
    }

    [[nodiscard]] inline auto GetDateTimeFormat() noexcept
    {
        return Get("global").GetDateTimeFormat();
    }

    [[nodiscard]] inline auto GetFormat() noexcept -> std::string
    {
        return Get("global").GetFormat();
    }

    [[nodiscard]] inline auto GetDefaultInfoStyle() noexcept
    {
        return Get("global").GetDefaultInfoStyle();
    }

    [[nodiscard]] inline auto GetDefaultWarnStyle() noexcept
    {
        return Get("global").GetDefaultWarnStyle();
    }

    [[nodiscard]] inline auto GetDefaultErrorStyle() noexcept
    {
        return Get("global").GetDefaultErrorStyle();
    }

    [[nodiscard]] inline auto GetDefaultFatalStyle() noexcept
    {
        return Get("global").GetDefaultFatalStyle();
    }

    inline void SetDefaultPrefix(const std::string_view newDefaultPrefix) noexcept
    {
        Get("global").SetDefaultPrefix(newDefaultPrefix);
    }

    inline void SetDateTimeFormat(const std::string_view newDateTimeFormat) noexcept
    {
        Get("global").SetDateTimeFormat(newDateTimeFormat);
    }

    inline void SetFormat(const std::string_view newFormat) noexcept
    {
        Get("global").SetFormat(newFormat);
    }

    inline void SetDefaultInfoStyle(const fmt::text_style& style) noexcept
    {
        Get("global").SetDefaultInfoStyle(style);
    }

    inline void SetDefaultWarnStyle(const fmt::text_style& style) noexcept
    {
        Get("global").SetDefaultWarnStyle(style);
    }

    inline void SetDefaultErrorStyle(const fmt::text_style& style) noexcept
    {
        Get("global").SetDefaultErrorStyle(style);
    }

    inline void SetDefaultFatalStyle(const fmt::text_style& style) noexcept
    {
        Get("global").SetDefaultFatalStyle(style);
    }

    inline void Log(const LogMsg& log)
    {
        Get("global").Log(log);
    }

    template <typename... TArgs>
    inline void Log(const std::string_view prefix, const Level level, const fmt::text_style& style,
                    const std::string_view fmt, TArgs&&... args)
    {
        Get("global").Log(prefix, level, style, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Log(const LogMsg& log, const std::string_view fmt, TArgs&&... args)
    {
        Get("global").Log(log, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Log(const Level level, const std::string_view fmt, TArgs&&... args)
    {
        Get("global").Log(level, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Log(const std::string_view prefix, const Level level, const std::string_view fmt, TArgs&&... args)
    {
        Get("global").Log(prefix, level, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    void Log(const Level level, const fmt::text_style& style, const std::string_view fmt, TArgs&&... args)
    {
        Get("global").Log(level, style, fmt, std::forward<TArgs>(args)...);
    }
} // namespace lgx
