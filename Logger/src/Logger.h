#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>

#include <fmt/args.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>

namespace logex {
    class Logger
    {
    public:
        struct Properties
        {
            std::ostream&    output         = std::cout;
            bool             unicodeSymbols = false;
            bool             dateAndTime    = true;
            std::string      dateTimeFormat = "%Y:%m:%d:%H:%M:%S";
            std::string      suffix         = "App";
            std::string_view format         = "[{datetime}] [{type}] ({suffix}): {msg}\n";
        };

    private:
        Properties m_Properties;
        std::mutex m_Guard;

    public:
        Logger(Properties properties) noexcept
            : m_Properties(std::move(properties))
        {
        }
        ~Logger() noexcept {}

    private:
        // Helper function to check if a placeholder exists in the format string
        static constexpr bool ContainsPlaceholder(const std::string_view format,
                                                  const std::string_view placeholder) noexcept
        {
            return format.find(placeholder) != std::string_view::npos;
        }

        template <typename... TArgs>
        void Log(const fmt::color& color, const std::string_view type, const std::string_view fmt, TArgs&&... args)
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
            if (ContainsPlaceholder(m_Properties.format, "{type}"))
                arg_store.push_back(fmt::arg("type", type));
            if (ContainsPlaceholder(m_Properties.format, "{suffix}"))
                arg_store.push_back(fmt::arg("suffix", m_Properties.suffix));
            if (!ContainsPlaceholder(m_Properties.format, "{msg}"))
                throw std::invalid_argument("A message is always required.");

            // Always push the message argument
            arg_store.push_back(fmt::arg("msg", fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...)));

            if (&m_Properties.output == &std::cout)
            {
                fmt::vprint(stdout, fmt::fg(color), m_Properties.format, arg_store);
            }
            else
            {
                m_Properties.output << fmt::vformat(m_Properties.format, arg_store);
            }
        }

    public:
        template <typename... TArgs>
        void Info(const std::string_view fmt, TArgs&&... args)
        {
            Log(fmt::color::light_sky_blue, "Info", fmt, std::forward<TArgs>(args)...);
        }

        template <typename... TArgs>
        void Warn(const std::string_view fmt, TArgs&&... args)
        {
            Log(fmt::color::yellow, "Warning", fmt, std::forward<TArgs>(args)...);
        }

        template <typename... TArgs>
        void Error(const std::string_view fmt, TArgs&&... args)
        {
            Log(fmt::color::red, "Error", fmt, std::forward<TArgs>(args)...);
        }

        template <typename... TArgs>
        void Fatal(const std::string_view fmt, TArgs&&... args)
        {
            Log(fmt::color::dark_red, "Fatal", fmt, std::forward<TArgs>(args)...);
        }
    };

} // namespace logex
