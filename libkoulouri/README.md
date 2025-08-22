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