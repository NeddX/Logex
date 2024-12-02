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
            bool                       flushOnLog                  = false;
            std::string                defaultPrefix               = "App";
            std::string                dateTimeFormat              = "%Y-%m-%d %H:%M:%S";
            DefaultStyle               defaultStyle                = DefaultStyle{};
        };

    private:
        Properties         m_Properties;
        mutable std::mutex m_Guard;

    public:
        [[nodiscard]] inline auto GetOutputStreams() const noexcept -> const std::vector<std::ostream*>&
        {
            return m_Properties.outputStreams;
        }
        [[nodiscard]] inline auto GetDefaultPrefix() const noexcept -> std::string
        {
            return m_Properties.defaultPrefix;
        }
        [[nodiscard]] inline auto GetDateTimeFormat() const noexcept -> std::string
        {
            return m_Properties.dateTimeFormat;
        }
        [[nodiscard]] inline auto GetFormat() const noexcept -> std::string { return m_Properties.defaultStyle.format; }
        [[nodiscard]] inline auto GetDefaultInfoStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultStyle.defaultInfoStyle;
        }
        [[nodiscard]] inline auto GetDefaultWarnStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultStyle.defaultWarnStyle;
        }
        [[nodiscard]] inline auto GetDefaultErrorStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultStyle.defaultErrorStyle;
        }
        [[nodiscard]] inline auto GetDefaultFatalStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultStyle.defaultFatalStyle;
        }
        inline auto SetOutputStreams(std::vector<std::ostream*> oss) noexcept -> void
        {
            m_Properties.outputStreams = std::move(oss);
        }
        inline auto SetDefaultPrefix(const std::string_view newDefaultPrefix) noexcept -> void
        {
            m_Properties.defaultPrefix = newDefaultPrefix;
        }
        inline auto SetDateTimeFormat(const std::string_view newDateTimeFormat) noexcept -> void
        {
            m_Properties.dateTimeFormat = newDateTimeFormat;
        }
        inline auto SetFormat(const std::string_view newFormat) noexcept -> void
        {
            m_Properties.defaultStyle.format = newFormat;
        }
        inline auto SetDefaultInfoStyle(const fmt::text_style& newDefaultInfoStyle) noexcept -> void
        {
            m_Properties.defaultStyle.defaultInfoStyle = newDefaultInfoStyle;
        }
        inline auto SetDefaultWarnStyle(const fmt::text_style& newDefaultWarnStyle) noexcept -> void
        {
            m_Properties.defaultStyle.defaultWarnStyle = newDefaultWarnStyle;
        }
        inline auto SetDefaultErrorStyle(const fmt::text_style& newDefaultErrorStyle) noexcept -> void
        {
            m_Properties.defaultStyle.defaultErrorStyle = newDefaultErrorStyle;
        }
        inline auto SetDefaultFatalStyle(const fmt::text_style& newDefaultFatalStyle) noexcept -> void
        {
            m_Properties.defaultStyle.defaultFatalStyle = newDefaultFatalStyle;
        }

    public:
        Logger(Properties properties) noexcept
            : m_Properties(std::move(properties))
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
        auto Flush() const -> void {}
        auto Log(const LogMsg& log) const -> void
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

                    if (m_Properties.flushOnLog)
                        stream->flush();
                }
            }
        }
        template <typename... TArgs>
        auto Log(const std::string_view prefix, const Level level, const fmt::text_style& style,
                 const std::string_view fmt, TArgs&&... args) const -> void
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

                    if (m_Properties.flushOnLog)
                        stream->flush();
                }
            }
        }
        template <typename... TArgs>
        auto Log(LogMsg log, const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(log.prefix.value_or(m_Properties.defaultPrefix), log.level, log.style, fmt,
                std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        auto Log(const Level level, const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(m_Properties.defaultPrefix, level, DefaultStyleFromLevel(level), fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        auto Log(const std::string_view prefix, const Level level, const std::string_view fmt, TArgs&&... args) const
            -> void
        {
            Log(prefix, level, DefaultStyleFromLevel(level), fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        auto Log(const Level level, const fmt::text_style& style, const std::string_view fmt, TArgs&&... args) const
            -> void
        {
            Log(m_Properties.defaultPrefix, level, style, fmt, std::forward<TArgs>(args)...);
        }

    public:
        template <typename... TArgs>
        auto Info(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Info, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        auto Warn(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Warn, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        auto Error(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Error, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        auto Fatal(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Fatal, fmt, std::forward<TArgs>(args)...);
        }
    };

    namespace internal {
        extern Logger                                  g_GlobalLogger;
        extern std::unordered_map<std::string, Logger> g_Loggers;
    } // namespace internal

    [[nodiscard]] inline auto Get(const std::string& loggerName) -> Logger&
    {
        return internal::g_Loggers.at(loggerName);
    }

    inline auto New(const std::string& name, const Logger::Properties& properties) noexcept -> Logger&
    {
        auto [it, con] = internal::g_Loggers.emplace(name, properties);
        return it->second;
    }

    [[nodiscard]] inline auto GetDefaultPrefix() noexcept
    {
        return internal::g_GlobalLogger.GetDefaultPrefix();
    }

    [[nodiscard]] inline auto GetDateTimeFormat() noexcept
    {
        return internal::g_GlobalLogger.GetDateTimeFormat();
    }

    [[nodiscard]] inline auto GetFormat() noexcept -> std::string
    {
        return internal::g_GlobalLogger.GetFormat();
    }

    [[nodiscard]] inline auto GetDefaultInfoStyle() noexcept
    {
        return internal::g_GlobalLogger.GetDefaultInfoStyle();
    }

    [[nodiscard]] inline auto GetDefaultWarnStyle() noexcept
    {
        return internal::g_GlobalLogger.GetDefaultWarnStyle();
    }

    [[nodiscard]] inline auto GetDefaultErrorStyle() noexcept
    {
        return internal::g_GlobalLogger.GetDefaultErrorStyle();
    }

    [[nodiscard]] inline auto GetDefaultFatalStyle() noexcept
    {
        return internal::g_GlobalLogger.GetDefaultFatalStyle();
    }

    inline void SetDefaultPrefix(const std::string_view newDefaultPrefix) noexcept
    {
        internal::g_GlobalLogger.SetDefaultPrefix(newDefaultPrefix);
    }

    inline void SetDateTimeFormat(const std::string_view newDateTimeFormat) noexcept
    {
        internal::g_GlobalLogger.SetDateTimeFormat(newDateTimeFormat);
    }

    inline void SetFormat(const std::string_view newFormat) noexcept
    {
        internal::g_GlobalLogger.SetFormat(newFormat);
    }

    inline void SetDefaultInfoStyle(const fmt::text_style& style) noexcept
    {
        internal::g_GlobalLogger.SetDefaultInfoStyle(style);
    }

    inline void SetDefaultWarnStyle(const fmt::text_style& style) noexcept
    {
        internal::g_GlobalLogger.SetDefaultWarnStyle(style);
    }

    inline void SetDefaultErrorStyle(const fmt::text_style& style) noexcept
    {
        internal::g_GlobalLogger.SetDefaultErrorStyle(style);
    }

    inline void SetDefaultFatalStyle(const fmt::text_style& style) noexcept
    {
        internal::g_GlobalLogger.SetDefaultFatalStyle(style);
    }

    inline auto Flush() -> void
    {
        internal::g_GlobalLogger.Flush();
    }

    inline auto Log(const LogMsg& log) -> void
    {
        internal::g_GlobalLogger.Log(log);
    }

    template <typename... TArgs>
    inline auto Log(const std::string_view prefix, const Level level, const fmt::text_style& style,
                    const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Log(prefix, level, style, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Log(const LogMsg& log, const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Log(log, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Log(const Level level, const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Log(level, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Log(const std::string_view prefix, const Level level, const std::string_view fmt, TArgs&&... args)
        -> void
    {
        internal::g_GlobalLogger.Log(prefix, level, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    auto Log(const Level level, const fmt::text_style& style, const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Log(level, style, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Info(const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Info(fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Warn(const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Warn(fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Error(const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Error(fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Fatal(const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Fatal(fmt, std::forward<TArgs>(args)...);
    }
} // namespace lgx
