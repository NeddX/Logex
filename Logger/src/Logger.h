#include "Common.h"

namespace lgx {
    class Logger
    {
    public:
        struct Properties
        {
            std::ostream&   outputStream                = std::cout;
            bool            unicodeSymbols              = false;
            bool            serializeOnNonStdoutStreams = false;
            std::string     defaultPrefix               = "App";
            std::string     dateTimeFormat              = "%Y-%m-%d %H:%M:%S";
            std::string     format                      = "[{datetime}] [{level}] ({prefix}): {msg}\n";
            fmt::text_style defaultInfoStyle            = fmt::bg(fmt::color::dark_green) | fmt::fg(fmt::color::white);
            fmt::text_style defaultWarnStyle            = fmt::bg(fmt::color::orange) | fmt::fg(fmt::color::black);
            fmt::text_style defaultErrorStyle =
                fmt::emphasis::italic | fmt::bg(fmt::color::red) | fmt::fg(fmt::color::white);
            fmt::text_style defaultFatalStyle =
                fmt::emphasis::italic | fmt::bg(fmt::color::dark_red) | fmt::fg(fmt::color::white);
        };

    private:
        Properties         m_Properties;
        mutable std::mutex m_Guard;

    public:
        [[nodiscard]] inline auto GetDefaultPrefix() const noexcept -> std::string
        {
            return m_Properties.defaultPrefix;
        }
        [[nodiscard]] inline auto GetDateTimeFormat() const noexcept -> std::string
        {
            return m_Properties.dateTimeFormat;
        }
        [[nodiscard]] inline auto GetFormat() const noexcept -> std::string { return m_Properties.format; }
        [[nodiscard]] inline auto GetDefaultInfoStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultInfoStyle;
        }
        [[nodiscard]] inline auto GetDefaultWarnStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultWarnStyle;
        }
        [[nodiscard]] inline auto GetDefaultErrorStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultErrorStyle;
        }
        [[nodiscard]] inline auto GetDefaultFatalStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultFatalStyle;
        }
        inline void SetDefaultPrefix(const std::string_view newDefaultPrefix) noexcept
        {
            m_Properties.defaultPrefix = newDefaultPrefix;
        }
        inline void SetDateTimeFormat(const std::string_view newDateTimeFormat) noexcept
        {
            m_Properties.dateTimeFormat = newDateTimeFormat;
        }
        inline void SetFormat(const std::string_view newFormat) noexcept { m_Properties.format = newFormat; }
        inline void SetDefaultInfoStyle(const fmt::text_style& newDefaultInfoStyle) noexcept
        {
            m_Properties.defaultInfoStyle = newDefaultInfoStyle;
        }
        inline void SetDefaultWarnStyle(const fmt::text_style& newDefaultWarnStyle) noexcept
        {
            m_Properties.defaultWarnStyle = newDefaultWarnStyle;
        }
        inline void SetDefaultErrorStyle(const fmt::text_style& newDefaultErrorStyle) noexcept
        {
            m_Properties.defaultErrorStyle = newDefaultErrorStyle;
        }
        inline void SetDefaultFatalStyle(const fmt::text_style& newDefaultFatalStyle) noexcept
        {
            m_Properties.defaultFatalStyle = newDefaultFatalStyle;
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

                case Info: return m_Properties.defaultInfoStyle;
                case Warn: return m_Properties.defaultWarnStyle;
                case Error: return m_Properties.defaultErrorStyle;
                case Fatal: return m_Properties.defaultFatalStyle;
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

            if (ContainsPlaceholder(m_Properties.format, "{datetime}"))
            {
                arg_store.push_back(
                    fmt::arg("datetime", fmt::format(fmt::runtime("{:" + m_Properties.dateTimeFormat + '}'),
                                                     fmt::localtime(time_obj))));
            }
            if (ContainsPlaceholder(m_Properties.format, "{level}"))
                arg_store.push_back(fmt::arg("level", log.level));
            if (ContainsPlaceholder(m_Properties.format, "{prefix}"))
                arg_store.push_back(fmt::arg("prefix", log.prefix.value_or(m_Properties.defaultPrefix)));
            if (!ContainsPlaceholder(m_Properties.format, "{msg}"))
                throw std::invalid_argument("A message is always required.");

            // Always push the message argument
            arg_store.push_back(fmt::arg("msg", log.message));

            if (&m_Properties.outputStream == &std::cout)
            {
                fmt::vprint(stdout, log.style, m_Properties.format, arg_store);
            }
            else
            {
                if (m_Properties.serializeOnNonStdoutStreams)
                    m_Properties.outputStream << LogMsg::ToString(log) << '\n';
                else
                    m_Properties.outputStream << fmt::vformat(m_Properties.format, arg_store);
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

            if (ContainsPlaceholder(m_Properties.format, "{datetime}"))
            {
                arg_store.push_back(
                    fmt::arg("datetime", fmt::format(fmt::runtime("{:" + m_Properties.dateTimeFormat + '}'),
                                                     fmt::localtime(time_obj))));
            }
            if (ContainsPlaceholder(m_Properties.format, "{level}"))
                arg_store.push_back(fmt::arg("level", level));
            if (ContainsPlaceholder(m_Properties.format, "{prefix}"))
                arg_store.push_back(fmt::arg("prefix", prefix));
            if (!ContainsPlaceholder(m_Properties.format, "{msg}"))
                throw std::invalid_argument("A message is always required.");

            // Always push the message argument
            arg_store.push_back(fmt::arg("msg", fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...)));

            if (&m_Properties.outputStream == &std::cout)
            {
                fmt::vprint(stdout, style, m_Properties.format, arg_store);
            }
            else
            {
                if (m_Properties.serializeOnNonStdoutStreams)
                {
                    const auto log = LogMsg{ .level   = level,
                                             .message = fmt::vformat(m_Properties.format, arg_store),
                                             .prefix  = std::string{ prefix },
                                             .style   = style };
                    m_Properties.outputStream << LogMsg::ToString(log) << '\n';
                }
                else
                    m_Properties.outputStream << fmt::vformat(m_Properties.format, arg_store);
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

    inline void Log(const LogMsg& log)
    {
        internal::g_GlobalLogger.Log(log);
    }

    template <typename... TArgs>
    inline void Log(const std::string_view prefix, const Level level, const fmt::text_style& style,
                    const std::string_view fmt, TArgs&&... args)
    {
        internal::g_GlobalLogger.Log(prefix, level, style, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Log(const LogMsg& log, const std::string_view fmt, TArgs&&... args)
    {
        internal::g_GlobalLogger.Log(log, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Log(const Level level, const std::string_view fmt, TArgs&&... args)
    {
        internal::g_GlobalLogger.Log(level, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Log(const std::string_view prefix, const Level level, const std::string_view fmt, TArgs&&... args)
    {
        internal::g_GlobalLogger.Log(prefix, level, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    void Log(const Level level, const fmt::text_style& style, const std::string_view fmt, TArgs&&... args)
    {
        internal::g_GlobalLogger.Log(level, style, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Info(const std::string_view fmt, TArgs&&... args)
    {
        internal::g_GlobalLogger.Info(fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Warn(const std::string_view fmt, TArgs&&... args)
    {
        internal::g_GlobalLogger.Warn(fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Error(const std::string_view fmt, TArgs&&... args)
    {
        internal::g_GlobalLogger.Error(fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline void Fatal(const std::string_view fmt, TArgs&&... args)
    {
        internal::g_GlobalLogger.Fatal(fmt, std::forward<TArgs>(args)...);
    }
} // namespace lgx
