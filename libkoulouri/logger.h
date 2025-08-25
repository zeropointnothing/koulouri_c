#pragma once
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <fstream>
#include <memory>

/**
 * Basic extension of the Logger class to allow for mirroring (or sole) file logging.
 */
class FileSink {
public:
    explicit FileSink(const std::string& path);
    ~FileSink();

    static bool isWritable(const std::string& path) {
        std::ofstream test(path, std::ios::app);
        return test.is_open();
    }

    /**
     * Attempt to write data to the file.
     * @param message The message to write
     */
    void write(const std::string& message) const;
    // Whether the contained file stream is still valid.
    [[nodiscard]] bool isValid() const;
    [[nodiscard]] std::ostream* stream() const;

private:
    std::unique_ptr<std::ofstream> file_;
    std::string path_;
};



class Logger {
public:
    enum class Level {
        DEBUG, // developer info
        INFO, // general info
        WARNING, // program warnings - can be ignored
        ERROR, // program errors - may be recoverable, and ignored
        CRITICAL // program critical errors - typically unrecoverable and should not be ignored
    };

    static std::string levelString(Level level);

    static void log(Level level, std::string_view message);
    static void setVerbosity(Level level);
    static void setCallback(const std::function<void(Level, std::string_view)> &cb);

    /**
     * Set primary output. Typically, cout/cerr, or similar.
     * @param out The ostream to write to
     */
    static void setOutput(std::ostream* out);

    /**
     * Set the FileSink. Typically used as a secondary mirror, though can be used as a primary logging location.
     * @param sink The FileSink object to write to
     */
    static void setSink(FileSink* sink);

    /**
     * Set the 'module' name. Should be short, yet descriptive. Used ScopedModuleName if you aren't on a new thread.
     * @param module
     */
    static void setModule(std::string_view module);
    // Get the current 'module' name.
    static std::string getModule();

private:
    static std::mutex mutex_; // thread-safe lock
    static std::ostream* out_; // primary output
    static FileSink* sink_; // secondary output
    static thread_local std::string module_; // current (thread) module
    static Level verbosity_; // minimum level required for logging
    static std::function<void(Level, std::string_view)> cb_; // logger override
};

/**
 * Basic module name scope manager.
 *
 * Automatically sets the 'module name' for the logger for this thread, then sets it back once destroyed.
 * Note, this is only required when you need to change the module name for the same thread, as the variable is defined
 * with thread_local.
 */
class ScopedModuleName {
public:
    explicit ScopedModuleName(std::string_view newName)
        : previous_(Logger::getModule()) {
        Logger::setModule(newName);
    }

    ~ScopedModuleName() {
        Logger::setModule(previous_);
    }

private:
    std::string previous_;
};
