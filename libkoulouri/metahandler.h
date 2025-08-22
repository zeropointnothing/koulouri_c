#pragma once
#include <vector>
#include <string>
#include <unordered_map>

class Track
{
public:
    std::string title;
    std::string artist;
    std::string album;
    std::string id;
    int trackNumber;
    const std::string filePath;

    explicit Track(const std::string &path);

    bool load();
};

class MetaCache
{
public:
    bool dumpCache(std::string &path) const;
    bool loadCache(std::string &path);

    void addTrack(Track &track);

    void setCache(std::unordered_map<std::string, Track>&& newCache);
    const std::unordered_map<std::string, Track>& getCache() const;

private:
    std::unordered_map<std::string, Track> cache_;
};

class MetaHandler
{
public:
    MetaHandler();
    std::vector<std::string> fetchAudioFiles(const std::string &directoryPath);
    std::vector<Track> loadTrackFromDirectory(const std::string &directoryPath);

    void populateMetaCache(const std::string &directoryPath, MetaCache *originalCache);


private:

};
