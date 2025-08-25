#include "logger.h"

#include <iostream>
#include <ostream>
#include <string>

FileSink::FileSink(const std::string& path)
    : file_(std::make_unique<std::ofstream>(path, std::ios::out | std::ios::trunc))
    , path_(path) {
    if (!file_->is_open()) {
        file_.reset();
    }
}

FileSink::~FileSink() {
    if (file_ && file_->is_open()) {
        file_->flush();
        file_->close();
    }
}

bool FileSink::isValid() const {
    return file_ && file_->is_open();
}

void FileSink::write(const std::string &message) const {
    if (file_ && file_->is_open()) {
        (*file_) << message << std::endl;
    } else {
        std::cerr << "[FileMirror] Warning: File stream unavailable for " << path_ << std::endl;
    }
}


std::ostream* FileSink::stream() const {
    return file_.get();
}


std::string Logger::levelString(const Level level) {
    switch (level) {
        case Level::INFO: {return "INFO";}
        case Level::DEBUG: {return "DEBUG";}
        case Level::WARNING: {return "WARNING";}
        case Level::ERROR: {return "ERROR";}
        case Level::CRITICAL: {return "CRITICAL";}
    }
    return "???";
}

std::ostream* Logger::out_ = nullptr;
FileSink* Logger::sink_ = nullptr;
std::function<void(Logger::Level, std::string_view)> Logger::cb_ = nullptr;
std::mutex Logger::mutex_;
Logger::Level Logger::verbosity_ = Level::INFO;

/**
 * Create a new logger instance. Acts as a wrapper for static log calls.
 *
 * Note, if you need to quickly log something, you can also use g_log to log globally.
 * @param moduleName The name to identify the logger
 */
Logger::Logger(const std::string &moduleName) {
    module_ = moduleName;
};


void Logger::log(const Level level, const std::string_view message) const {
    g_log(module_, level, message);
}
void Logger::log(const Level level, const std::string_view sub, const std::string_view message) const {
    g_log(module_, level, sub, message);
}


void Logger::setOutput(std::ostream *out) {
    out_ = out;
}
void Logger::setSink(FileSink *sink) {
    if (sink != nullptr && !sink->isValid()) {
        g_log("FileSink", Level::CRITICAL, "Requested sink is invalid!");
        return;
    }
    sink_ = sink;
}

void Logger::setVerbosity(const Level level) {
    verbosity_ = level;
}
void Logger::setCallback(const std::function<void(Level, std::string_view)> &cb) {
    cb_ = cb;
}

void Logger::g_log(std::string_view module, const Level level, const std::string_view message) {
    if (cb_ != nullptr) {
        cb_(level, message);
    } else {
        std::lock_guard lock(mutex_); // thread safety
        const std::string msg = "[" + std::string(module.data()) + "::" + levelString(level) + "] " + message.data();

        if (out_ != nullptr && level >= verbosity_) {
            (*out_) << msg << std::endl;
        }
        if (sink_ != nullptr && level >= verbosity_) {
            sink_->write(msg);
        }
    }
}

void Logger::g_log(std::string_view module, const Level level, const std::string_view sub, const std::string_view message) {
    if (cb_ != nullptr) {
        cb_(level, message);
    } else {
        std::lock_guard lock(mutex_); // thread safety
        const std::string msg = "[" + std::string(module.data()) + "." + sub.data() + "::" + levelString(level) + "] " + message.data();

        if (out_ != nullptr && level >= verbosity_) {
            (*out_) << msg << std::endl;
        }
        if (sink_ != nullptr && level >= verbosity_) {
            sink_->write(msg);
        }
    }
}


