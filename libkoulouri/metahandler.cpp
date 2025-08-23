#include "metahandler.h"
#include "taglib/fileref.h"
#include "taglib/tag.h"
#include <filesystem>
#include <iostream>
#include <fstream>

Track::Track(const std::string &path) : filePath(path) {}

bool Track::load() {
    TagLib::FileRef f(filePath.c_str());
    if (!f.isNull() && f.tag()) {
        title  = f.tag()->title().to8Bit(true);
        artist = f.tag()->artist().to8Bit(true);
        album  = f.tag()->album().to8Bit(true);
        trackNumber = f.tag()->track();

        if (title.empty() && artist.empty() && album.empty()) {
            std::cerr << "Failed to parse metadata: No artist, no title, no album, no service. | " << filePath << std::endl;
            return false;
        }
        return true;

    }
    return false;
}

std::string generateTrackID(const Track &track) {
    std::string combined = track.album + "|" + track.title + "|" + track.artist;
    std::size_t hash = std::hash<std::string>{}(combined);
    std::ostringstream oss;
    oss << std::hex << hash;
    return oss.str();
}


// MetaCache
void MetaCache::setCache(std::unordered_map<std::string, Track>&& newCache) {
    cache_ = std::move(newCache);
}

const std::unordered_map<std::string, Track>& MetaCache::getCache() const {
    return cache_;
}

/**
 * Sort the MetaCache via a custom callback.
 *
 * Note, for integrity, all Tracks will be returned as const pointers.
 * @param key The function to sort with
 * @return
 */
std::vector<const Track*> MetaCache::sortBy(const SortKey &key) const {
    std::vector<const Track*> result; // ensure integrity by forbidding changes
    result.reserve(cache_.size());
    for (const auto &[id, track] : cache_) {
        result.push_back(&track);
    }
    std::sort(result.begin(), result.end(), [&](const Track* a, const Track* b) {
        return key(*a,*b);
    });
    return result;
}

bool MetaCache::dumpCache(std::string &path) const {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    for (const auto& [path, track] : cache_) {
        auto write_string = [&](const std::string& str) {
            size_t len = str.size();
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            out.write(str.data(), len);
        };

        write_string(track.filePath);
        write_string(track.title);
        write_string(track.artist);
        write_string(track.album);
        write_string(track.id);
        out.write(reinterpret_cast<const char*>(&track.trackNumber), sizeof(track.trackNumber));
        // out.write(reinterpret_cast<const char*>(&track.duration), sizeof(track.duration));
    }

    return true;
};

bool MetaCache::loadCache(std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    cache_.clear();

    while (in.good()) {
        size_t len;
        std::string path, title, artist, album, id;
        int trackNumber;

        // Read path
        if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
        path.resize(len);
        if (!in.read(&path[0], len)) break;

        // Read title
        if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
        title.resize(len);
        if (!in.read(&title[0], len)) break;

        // Read artist
        if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
        artist.resize(len);
        if (!in.read(&artist[0], len)) break;

        // Read album
        if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
        album.resize(len);
        if (!in.read(&album[0], len)) break;

        // Read trackId
        if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
        id.resize(len);
        if (!in.read(&id[0], len)) break;

        // Read trackNumer
        if (!in.read(reinterpret_cast<char*>(&trackNumber), sizeof(trackNumber))) break;

        Track track(path);

        track.title       = title;
        track.artist      = artist;
        track.album       = album;
        track.trackNumber = trackNumber;
        track.id          = id;

        addTrack(track);
    }

    return true;
}

void MetaCache::addTrack(Track &track) {
    cache_.insert({track.id, track});
}

namespace fs = std::filesystem;
MetaHandler::MetaHandler() {}

/**
 * @brief Fetch a vector of all (supported) files in a directory.
 * @param directoryPath: The directory to scan
 * @return
 */
std::vector<std::string> MetaHandler::fetchAudioFiles(const std::string& directoryPath) {
    std::vector<std::string> audioFiles;
    const std::vector<std::string> supportedExtensions = {
        "mp3", "flac", "wav", "ogg", "m4a", "aac", "aiff", "wma"
    };

    for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (!ext.empty() && ext.front() == '.') ext.erase(0, 1); // remove leading dot
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (std::find(supportedExtensions.begin(), supportedExtensions.end(), ext) != supportedExtensions.end()) {
                audioFiles.push_back(entry.path().string());
            }
        }
    }

    return audioFiles;
}

/**
 * @brief Attempt to create a vector of Track objects corresponding to all user media.
 * @param directoryPath
 * @return
 */
std::vector<Track> MetaHandler::loadTrackFromDirectory(const std::string &directoryPath) {
    std::vector<Track> tracks;
    std::vector<std::string> files = fetchAudioFiles(directoryPath);

    for (const std::string &path: files) {
        Track track(path);
        if (track.load()) {
            track.id = generateTrackID(track);
            tracks.push_back(track);
        } else {
            std::cerr << "Failed to load metadata for file: " << path << std::endl;
        }
    }

    return tracks;
}

void MetaHandler::populateMetaCache(const std::string &directoryPath, MetaCache *originalCache) {
    std::vector<std::string> files = fetchAudioFiles(directoryPath);

    int i = 0;
    for (const std::string &path : files) {
        Track track(path);
        if (track.load()) {
            track.id = generateTrackID(track);
            originalCache->addTrack(track);
            i += 1;
        } else {
            std::cerr << "Failed to load metadata for file: " << path << std::endl;
        }
    }
}
