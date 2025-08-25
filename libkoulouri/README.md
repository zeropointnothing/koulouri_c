# libKoulouri

A (mini)library containing various useful wrappers and helpers for
playing and managing music.

## Player

The main logic powering libKoulouri.
Provides useful libsndfile/PortAudio wrappers to play audio.

---

### AudioPlayer
Main class. Provides many functions and utilities
for playing user audio.

### PlayerActionResult/Enum
Provides a way for libKoulouri to return useful information.

### FfmpegFile
Provides an interface for converting files via FFmpeg.

## Logger

Internal logger class used by libkoulouri and built in frontends.

### Logger
Logger class.

All instances of this class log via the same static methods (g_log), including libkoulouri.

Note that while libkoulouri will always use this interface to log information,
it is up to the frontend developer (you, likely) to set the output and/or file sink.
By default, all logs will be swallowed unless you at the very least call:

```c++
Logger::setOutput(&std::cout);
```

Alternatively, you may wish to disable this logger altogether. If desired, you may direct
all (g_)log calls to your own logger via the following method:
```c++
Logger::setCallback([](Logger::Level level, std::string_view message) {
    // your logger code here
});
```

### FileSink
Minimal 'file sink', allowing for logs to be written to files.

## FormatTools

Collection of utilities allowing libKoulouri to play/manage
audio dynamically.

---

### FormatType
Basic Enum to identify formats clearly.

### AudioBuffer
Vector wrapper, allowing for dynamic vector creation.

### FormatReader
Utility class to simplify reading data with libsndfile.

### FormatTools
Format conversion tools.