#include "Common.h"

namespace lgx {
    class Logger
    {
    public:
        struct DefaultStyle
        {
            std::string     format           = "[{datetime}] [{level}] ({prefix}): {msg}";
            fmt::text_style defaultInfoStyle = fmt::bg(fmt::color::dark_green) | fmt::fg(fmt::color::white);
            fmt::text_style defaultWarnStyle = fmt::bg(fmt::color::orange) | fmt::fg(fmt::color::black);
            fmt::text_style defaultErrorStyle =
                fmt::emphasis::italic | fmt::bg(fmt::color::red) | fmt::fg(fmt::color::white);
            fmt::text_style defaultFatalStyle =
                fmt::emphasis::italic | fmt::bg(fmt::color::dark_red) | fmt::fg(fmt::color::white);
            fmt::text_style defaultDebugStyle =
                fmt::emphasis::italic | fmt::bg(fmt::color::green) | fmt::fg(fmt::color::white);
            fmt::text_style defaultVerboseStyle =
                fmt::emphasis::italic | fmt::bg(fmt::color::gray) | fmt::fg(fmt::color::white);
        };
        struct Properties
        {
            std::string                loggerName                   = "Logex";
            Type                       appType                      = Type::User;
            std::vector<std::ostream*> outputStreams                = { &std::cout };
            bool                       serializeToNonStdoutStreams  = false;
            bool                       writeStyleToNonStdoutStreams = false;
            bool                       verbose                      = false;
            bool                       syslog                       = false;
            std::string                defaultPrefix                = "App";
            std::string                dateTimeFormat               = "%Y-%m-%d %H:%M:%S";
            DefaultStyle               defaultStyle                 = DefaultStyle{};
        };

    private:
        Properties                      m_Properties;
        mutable std::future<void>       m_PollThread;
        mutable std::condition_variable m_PollCV;
        mutable std::atomic<bool>       m_Run;
        mutable std::deque<LogMsg>      m_LogQueue;
        mutable std::mutex              m_QueueMutex;

    public:
        [[nodiscard]] inline auto GetOutputStreams() const noexcept -> const std::vector<std::ostream*>&
        {
            return m_Properties.outputStreams;
        }
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
        [[nodiscard]] inline auto GetSyslog() const noexcept -> bool
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            return m_Properties.syslog;
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
        [[nodiscard]] inline auto GetDefaultDebugStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultStyle.defaultDebugStyle;
        }
        [[nodiscard]] inline auto GetDefaultVerboseStyle() const noexcept -> fmt::text_style
        {
            return m_Properties.defaultStyle.defaultVerboseStyle;
        }
        inline auto SetOutputStreams(std::vector<std::ostream*> oss) noexcept -> void
        {
            m_Properties.outputStreams = std::move(oss);
        }
        inline auto SetDefaultPrefix(const std::string_view newDefaultPrefix) noexcept -> void
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultPrefix = newDefaultPrefix;
        }
        inline auto SetDateTimeFormat(const std::string_view newDateTimeFormat) noexcept -> void
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.dateTimeFormat = newDateTimeFormat;
        }
        inline auto SetFormat(const std::string_view newFormat) noexcept -> void
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.format = newFormat;
        }
        inline auto SetDefaultInfoStyle(const fmt::text_style& newDefaultInfoStyle) noexcept -> void
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.defaultInfoStyle = newDefaultInfoStyle;
        }
        inline auto SetDefaultWarnStyle(const fmt::text_style& newDefaultWarnStyle) noexcept -> void
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.defaultWarnStyle = newDefaultWarnStyle;
        }
        inline auto SetDefaultErrorStyle(const fmt::text_style& newDefaultErrorStyle) noexcept -> void
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.defaultErrorStyle = newDefaultErrorStyle;
        }
        inline auto SetDefaultFatalStyle(const fmt::text_style& newDefaultFatalStyle) noexcept -> void
        {
            const std::lock_guard<std::mutex> lock{ m_Guard };
            m_Properties.defaultStyle.defaultFatalStyle = newDefaultFatalStyle;
        }
        inline auto SetDefaultDebugStyle(const fmt::text_style& newDefaultDebugStyle) noexcept -> void
        {
            m_Properties.defaultStyle.defaultDebugStyle = newDefaultDebugStyle;
        }
        inline auto SetDefaultVerboseStyle(const fmt::text_style& newDefaultVerboseStyle) noexcept -> void
        {
            m_Properties.defaultStyle.defaultVerboseStyle = newDefaultVerboseStyle;
        }
        inline auto SetVerbose(const bool enable) noexcept -> void { m_Properties.verbose = enable; }
        inline auto SetSyslog(const bool enable) noexcept -> void { m_Properties.syslog = enable; }

    public:
        Logger() noexcept {}
        Logger(Properties properties) noexcept
            : m_Properties(std::move(properties))
        {
            m_Run.store(true);
            m_PollThread = std::async(std::launch::async, &Logger::PollLogs, this);
        }
        ~Logger() noexcept
        {
            m_Run.store(false);
            m_PollCV.notify_all();

            if (m_PollThread.valid())
                m_PollThread.get();
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

                default:
                case Info: return m_Properties.defaultStyle.defaultInfoStyle;
                case Warn: return m_Properties.defaultStyle.defaultWarnStyle;
                case Error: return m_Properties.defaultStyle.defaultErrorStyle;
                case Fatal: return m_Properties.defaultStyle.defaultFatalStyle;
                case Debug: return m_Properties.defaultStyle.defaultDebugStyle;
                case Verbose: return m_Properties.defaultStyle.defaultVerboseStyle;
            }
        }

    private:
        [[nodiscard]] static constexpr auto ContainsPlaceholder(const std::string_view format,
                                                                const std::string_view placeholder) noexcept -> bool
        {
            return format.find(placeholder) != std::string_view::npos;
        }

    private:
        void PollLogs()
        {
#ifdef __unix__
            if (m_Properties.syslog)
            {
                int facility = LOG_USER;
                switch (m_Properties.appType)
                {
                    case Type::User: facility = LOG_USER; break;
                    case Type::Daemon: facility = LOG_DAEMON; break;
                    default: break;
                }
                openlog(m_Properties.loggerName.c_str(), LOG_PID | LOG_CONS, facility);
            }
#endif

            while (m_Run.load())
            {
                std::unique_lock<std::mutex> guard{ m_QueueMutex };
                m_PollCV.wait(guard, [this]() { return !m_LogQueue.empty() || !m_Run.load(); });

                if (!m_LogQueue.empty())
                {
                    auto log = m_LogQueue.front();
                    m_LogQueue.pop_front();
                    InternalLog(std::move(log));
                }
            }

#ifdef __unix__
            if (m_Properties.syslog)
                closelog();
#endif
        }

    private:
        inline auto InternalLog(const LogMsg& log) const -> void
        {
#ifndef LGX_DEBUG
            if (log.level == Level::Debug)
                return;
#endif

            if (log.level == Level::Verbose && !m_Properties.verbose)
                return;

            // Prepare arguments conditionally based on the presence of placeholders in the format string.
            auto arg_store = fmt::dynamic_format_arg_store<fmt::format_context>{};

            if (ContainsPlaceholder(m_Properties.defaultStyle.format, "{datetime}"))
            {
                auto time_now = std::chrono::system_clock::now();
                auto time_obj = std::chrono::system_clock::to_time_t(time_now);
                arg_store.push_back(
                    fmt::arg("datetime", fmt::format(fmt::runtime("{:" + m_Properties.dateTimeFormat + '}'),
                                                     fmt::localtime(std::move(time_obj)))));
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
                {
                    std::cout << fmt::vformat(log.style, m_Properties.defaultStyle.format, arg_store) << std::endl;
                }
                else
                {
                    if (m_Properties.serializeToNonStdoutStreams)
                        *stream << LogMsg::ToString(log) << std::endl;
                    else if (m_Properties.writeStyleToNonStdoutStreams)
                        *stream << fmt::vformat(log.style, m_Properties.defaultStyle.format, arg_store) << std::endl;
                    else
                        *stream << fmt::vformat(m_Properties.defaultStyle.format, arg_store) << std::endl;
                }
            }

#ifdef __unix__
            if (m_Properties.syslog)
            {
                std::string syslog_fmt = "{msg}";

                // Prepare arguments conditionally based on the presence of placeholders in the format string.
                auto arg_store = fmt::dynamic_format_arg_store<fmt::format_context>{};

                if (ContainsPlaceholder(m_Properties.defaultStyle.format, "{prefix}"))
                {
                    syslog_fmt = "[{prefix}] " + syslog_fmt;
                    arg_store.push_back(fmt::arg("prefix", log.prefix.value_or(m_Properties.defaultPrefix)));
                }
                if (!ContainsPlaceholder(m_Properties.defaultStyle.format, "{msg}"))
                    throw std::invalid_argument("A message is always required.");

                // Always push the message argument
                arg_store.push_back(fmt::arg("msg", log.message));

                int level = LOG_INFO;
                switch (log.level)
                {
                    using enum Level;

                    case Info: level = LOG_INFO; break;
                    case Warn: level = LOG_WARNING; break;
                    case Error: level = LOG_ERR; break;
                    case Fatal: level = LOG_ALERT; break;
                    case Debug:
                    case Verbose: level = LOG_DEBUG; break;
                }

                std::string strlogmsg;
                if (m_Properties.writeStyleToNonStdoutStreams)
                    strlogmsg = fmt::vformat(log.style, syslog_fmt, arg_store);
                else
                    strlogmsg = fmt::vformat(syslog_fmt, arg_store);

                syslog(level, "%s", strlogmsg.c_str());
            }
#endif
        }

    public:
        inline auto Log(LogMsg log) const -> void
        {
            std::scoped_lock guard{ m_QueueMutex };
            m_LogQueue.push_back(std::move(log));
            m_PollCV.notify_one();
        }
        template <typename... TArgs>
        constexpr auto Log(std::string prefix, const Level level, const fmt::text_style& style,
                           const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(LogMsg{ .level   = level,
                        .message = fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...),
                        .prefix  = std::move(prefix),
                        .style   = style });
        }
        template <typename... TArgs>
        constexpr auto Log(LogMsg log, const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(log.prefix.value_or(m_Properties.defaultPrefix), log.level, log.style, fmt,
                std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr auto Log(const Level level, const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(m_Properties.defaultPrefix, level, DefaultStyleFromLevel(level), fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr auto Log(const std::string prefix, const Level level, const std::string_view fmt,
                           TArgs&&... args) const -> void
        {
            Log(std::move(prefix), level, DefaultStyleFromLevel(level), fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr auto Log(const Level level, const fmt::text_style& style, const std::string_view fmt,
                           TArgs&&... args) const -> void
        {
            Log(m_Properties.defaultPrefix, level, style, fmt, std::forward<TArgs>(args)...);
        }

    public:
        template <typename... TArgs>
        constexpr auto Info(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Info, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr auto Warn(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Warn, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr auto Error(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Error, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr auto Fatal(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Fatal, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr auto Debug(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Debug, fmt, std::forward<TArgs>(args)...);
        }
        template <typename... TArgs>
        constexpr auto Verbose(const std::string_view fmt, TArgs&&... args) const -> void
        {
            Log(Level::Verbose, fmt, std::forward<TArgs>(args)...);
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

    [[nodiscard]] inline auto GetDefaultDebugStyle() noexcept
    {
        return internal::g_GlobalLogger.GetDefaultDebugStyle();
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

    inline void SetDefaultDebugStyle(const fmt::text_style& style) noexcept
    {
        internal::g_GlobalLogger.SetDefaultDebugStyle(style);
    }

    inline void SetDefaultVerboseStyle(const fmt::text_style& style) noexcept
    {
        internal::g_GlobalLogger.SetDefaultVerboseStyle(style);
    }

    inline auto Log(const LogMsg& log) -> void
    {
        Get("global").Log(log);
    }

    template <typename... TArgs>
    inline auto Log(const std::string_view prefix, const Level level, const fmt::text_style& style,
                    const std::string_view fmt, TArgs&&... args) -> void
    {
        Get("global").Log(prefix, level, style, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Log(const LogMsg& log, const std::string_view fmt, TArgs&&... args) -> void
    {
        Get("global").Log(log, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Log(const Level level, const std::string_view fmt, TArgs&&... args) -> void
    {
        Get("global").Log(level, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto Log(const std::string_view prefix, const Level level, const std::string_view fmt, TArgs&&... args)
        -> void
    {
        Get("global").Log(prefix, level, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    auto Log(const Level level, const fmt::text_style& style, const std::string_view fmt, TArgs&&... args) -> void
    {
        Get("global").Log(level, style, fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto LogDebug(const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Debug(fmt, std::forward<TArgs>(args)...);
    }

    template <typename... TArgs>
    inline auto LogVerbose(const std::string_view fmt, TArgs&&... args) -> void
    {
        internal::g_GlobalLogger.Verbose(fmt, std::forward<TArgs>(args)...);
    }
} // namespace lgx
