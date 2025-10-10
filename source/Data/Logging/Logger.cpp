#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>

Logger &Logger::getInstance()
{
    static Logger instance;
    return instance;
}

Logger::Logger() : m_minLevel(Level::INFO)
{
    // Initialize with default settings
}

void Logger::initialize(const std::string &logFile)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_logFile = std::make_unique<std::ofstream>(logFile, std::ios::app);
    if (!m_logFile->is_open())
    {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
        m_logFile.reset();
    }
}

void Logger::log(Level level, const std::string &message)
{
    if (level < m_minLevel)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = levelToString(level);

    std::string logMessage = "[" + timestamp + "] [" + levelStr + "] " + message;

    // Output to console only for WARNING and above to reduce noise
    if (level >= Level::WARNING)
    {
        std::cout << logMessage << std::endl;
    }

    // Output to log file if available
    if (m_logFile && m_logFile->is_open())
    {
        *m_logFile << logMessage << std::endl;
        m_logFile->flush();
    }
}

void Logger::debug(const std::string &message)
{
    log(Level::DEBUG, message);
}

void Logger::info(const std::string &message)
{
    log(Level::INFO, message);
}

void Logger::warning(const std::string &message)
{
    log(Level::WARNING, message);
}

void Logger::error(const std::string &message)
{
    log(Level::ERROR, message);
}

void Logger::critical(const std::string &message)
{
    log(Level::CRITICAL, message);
}

void Logger::setMinLevel(Level level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLevel = level;
}

std::string Logger::levelToString(Level level) const
{
    switch (level)
    {
    case Level::DEBUG:
        return "DEBUG";
    case Level::INFO:
        return "INFO";
    case Level::WARNING:
        return "WARNING";
    case Level::ERROR:
        return "ERROR";
    case Level::CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}
