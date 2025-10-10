#pragma once
#include <string>
#include <fstream>
#include <memory>
#include <mutex>

/**
 * Logger class for consistent error handling and logging
 * Follows Singleton pattern for global access
 * Thread-safe logging with mutex protection
 */
class Logger
{
public:
    // Log levels
    enum class Level
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    // Get singleton instance
    static Logger &getInstance();

    // Initialize logger with log file
    void initialize(const std::string &logFile = "game.log");

    // Log methods
    void log(Level level, const std::string &message);
    void debug(const std::string &message);
    void info(const std::string &message);
    void warning(const std::string &message);
    void error(const std::string &message);
    void critical(const std::string &message);

    // Set minimum log level
    void setMinLevel(Level level);

private:
    // Private constructor for singleton
    Logger();

    // Log file stream
    std::unique_ptr<std::ofstream> m_logFile;

    // Minimum log level
    Level m_minLevel;

    // Mutex for thread safety
    mutable std::mutex m_mutex;

    // Helper methods
    std::string levelToString(Level level) const;
    std::string getCurrentTimestamp() const;
};
