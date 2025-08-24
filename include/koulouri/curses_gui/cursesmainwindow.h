#pragma once
#include <ncurses.h>
#include "libkoulouri/metahandler.h"
#include "libkoulouri/player.h"

enum WindowType {
    TrackList
};

class CursesMainWindow {
public:
    CursesMainWindow();
    ~CursesMainWindow();

    int main();
    int renderBaseUi(WindowType winType);
    static void cleanup();
private:
    AudioPlayer player;
    const Track *currentTrack;
    const std::vector<Track*> queue;
    bool running = true;
    WindowType windowType;
    std::string userInput;

    MetaHandler mhandler = MetaHandler();
    MetaCache mcache = MetaCache();

    int maxy;
    int maxx;
};

class MenuHandler {
public:
    using FuncCallback = std::function<int(CursesMainWindow *w, MenuHandler *handler)>;
    MenuHandler() = default;
    ~MenuHandler() = default;

    void registerCallback(WindowType type, const FuncCallback& callback);
    int call(WindowType type, CursesMainWindow *win, MenuHandler *handler);

private:
    std::unordered_map<WindowType, FuncCallback> _callbacks;
};
