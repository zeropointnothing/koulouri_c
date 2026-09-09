// #pragma once
// #include <atomic>
// #include <ncurses.h>
// #include <queue>
// #include <thread>
//
// #include "libkoulouri/metahandler.h"
// #include "libkoulouri/player.h"
//
// enum WindowType {
//     TrackList,
//     QueueList
// };
//
// class CursesMainWindow {
// public:
//     CursesMainWindow();
//     ~CursesMainWindow();
//
//     int main();
//     int renderBaseUi(WindowType winType);
//     void handleInternalQueue();
//     void handleBaseInput(int k);
//     static void cleanup();
// private:
//     AudioPlayer player;
//     const Track *currentTrack;
//     size_t queueIndex;
//     std::vector<const Track*> queue;
//     bool running = true;
//     std::string userInput;
//
//     MetaHandler mhandler = MetaHandler();
//     MetaCache mcache = MetaCache();
//
//     int maxy;
//     int maxx;
// };
//
// class MenuHandler {
// public:
//     using FuncCallback = std::function<int(CursesMainWindow *w, MenuHandler *handler)>;
//     MenuHandler() = default;
//     ~MenuHandler() = default;
//
//     void registerCallback(WindowType type, const FuncCallback& callback);
//     int call(WindowType type, CursesMainWindow *win, MenuHandler *handler);
//
//     WindowType windowType;
//
// private:
//     std::unordered_map<WindowType, FuncCallback> _callbacks;
// };
//


#include <atomic>
#include <queue>
#include <thread>

#include "libkoulouri/metahandler.h"
#include "libkoulouri/player.h"

class UIDispatcher {
public:
    UIDispatcher();
    ~UIDispatcher();

    void claimInput(std::thread::id id);
    void releaseInput(std::thread::id id);
    int getInput(std::thread::id id);

    void executeOnMain(const std::function<void(AudioPlayer&, MetaHandler&, MetaCache&)>& task);
    void drain(AudioPlayer& player, MetaHandler& handler, MetaCache& cache);

private:
    std::atomic<std::thread::id> claimedThread;
    std::mutex inputMutex;
    std::mutex queueMutex;
    std::queue<std::function<void(AudioPlayer&, MetaHandler&, MetaCache&)>> taskQueue;
    int lastKey = -1;
};

class CursesMainWindow {
public:

    CursesMainWindow();
    ~CursesMainWindow();

    int main();

private:
    AudioPlayer player;
    MetaHandler mhandler;
    MetaCache mcache;
};
