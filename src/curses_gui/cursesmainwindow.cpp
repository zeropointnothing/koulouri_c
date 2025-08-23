#include "curses_gui/cursesmainwindow.h"

#include <algorithm>
#include <iostream>
#include <ostream>
#include <string>
#include <thread>
#include <locale>

#include "libkoulouri/player.h"

CursesMainWindow::CursesMainWindow() {
    mhandler.populateMetaCache("/home/exii/Music", &mcache);
    userInput = "";
    windowType = WindowType::TrackList;
};
CursesMainWindow::~CursesMainWindow() = default;
void CursesMainWindow::cleanup() {
    endwin();
}

void MenuHandler::registerCallback(const WindowType type, const FuncCallback &callback) {
    // _callbacks.insert(type, callback);
    // _acallbacks.insert("a", callback);
    _callbacks[type] = callback;
}
int MenuHandler::call(const WindowType type, CursesMainWindow *win) {
    return _callbacks[type](win);
}


int CursesMainWindow::main() {
    setlocale(LC_ALL, ""); // ensure we can render Unicode
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, true);
    keypad(stdscr, true);
    auto menu_handler = MenuHandler();

    menu_handler.registerCallback(WindowType::TrackList, [](CursesMainWindow *win) {
        const std::vector<const Track*> byArtist = win->mcache.sortBy([](const Track& a, const Track& b) {
            return a.artist < b.artist;
        });

        int scrollOffset = 0;
        while (win->running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            refresh();

            int y, x = getmaxyx(stdscr, y, x);

            for (int i = 0; i < y-3; i++) {
                move(i+1, 1);
                try {
                    const Track* track = byArtist.at(i+scrollOffset);
                    std::string str = std::to_string(i+scrollOffset) + " " + track->artist + " - " + track->title;
                    addstr(str.c_str());
                    clrtoeol();
                } catch (std::out_of_range &e) { // should crash cleanly
                    endwin();
                    std::cerr << e.what() << '\n';
                    initscr();
                    win->running = false;
                }
            }

            move(y-2, 1);
            std::string userInputStr = ": " + win->userInput;
            addstr(userInputStr.c_str());
            clrtoeol();

            box(stdscr, 0, 0);

            int k = getch();
            if (k == ERR) {
                // Do nothing!
            } else if (k == 27) { // escape
                win->running = false;
            } else if (k == KEY_DOWN) {
                scrollOffset++;
                scrollOffset = std::clamp(scrollOffset, 0, static_cast<int>(byArtist.size())-y+3);
                clear();
            } else if (k == KEY_UP) {
                scrollOffset--;
                scrollOffset = std::clamp(scrollOffset, 0, static_cast<int>(byArtist.size())-y+1);
                clear();
            } else if (k == KEY_BACKSPACE) {
                if (!win->userInput.empty()) {
                    win->userInput.pop_back();
                }
            } else if (k == KEY_ENTER || k == 10) {
                try {
                    const long trackNumber = stol(win->userInput);
                    try {
                        win->player.stop();
                        endwin();
                        PlayerActionResult load = win->player.load(byArtist.at(trackNumber)->filePath, true);
                        if (load.result == PlayerActionEnum::PASS) {
                            win->player.setVolume(70);
                            win->player.play();
                        }
                        initscr();
                    } catch (std::out_of_range &e) {
                        // pass - was out of range
                    }

                    win->userInput.clear();
                    clear();
                } catch (std::invalid_argument &e) {
                    // pass, no need to do anything here
                }
            } else if (isascii(k)) {
                win->userInput += static_cast<char>(k);
            }

            // refreshes the screen
            move(0, 0);
        }

        win->player.stop();
        return 0;
    });

    menu_handler.call(WindowType::TrackList, this);
    running = true;
    const int result = menu_handler.call(WindowType::TrackList, this);

    endwin();
    player.stop();
    return result;
}

