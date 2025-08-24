#include "curses_gui/cursesmainwindow.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>
#include <thread>
#include <locale>
#include <sstream>

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

void rectangle(int y1, int x1, int y2, int x2)
{
    mvhline(y1, x1, 0, x2-x1);
    mvhline(y2, x1, 0, x2-x1);
    mvvline(y1, x1, 0, y2-y1);
    mvvline(y1, x2, 0, y2-y1);
    mvaddch(y1, x1, ACS_ULCORNER);
    mvaddch(y2, x1, ACS_LLCORNER);
    mvaddch(y1, x2, ACS_URCORNER);
    mvaddch(y2, x2, ACS_LRCORNER);
}

std::string formatTime(double seconds) {
    int totalSecs = static_cast<int>(seconds);
    int hours = totalSecs / 3600;
    int minutes = (totalSecs % 3600) / 60;
    int secs = totalSecs % 60;

    std::ostringstream oss;
    if (hours > 0) {
        oss << std::setw(2) << std::setfill('0') << hours << ":";
    }
    oss << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << secs;
    return oss.str();
}


int CursesMainWindow::renderBaseUi() {
    double positionSeconds = player.posToSeconds(player.getPos());
    double maxPositionSeconds = player.posToSeconds(player.getMaxPos());

    // box(stdscr, 0, 0);
    rectangle(0,0,maxy-2,maxx-1);

    // create song details
    if (player.isLoaded()) {
        std::string startTime = formatTime(positionSeconds);
        std::string endTime = formatTime(maxPositionSeconds);
        std::string timeLabel = startTime + "-" + endTime;
        std::string infoLabel = currentTrack->artist + " - " + currentTrack->title;

        const int timeLabelWidth = static_cast<int>(timeLabel.size()) + 1;
        const int barWidth = static_cast<int>((maxx - timeLabelWidth) * (positionSeconds / maxPositionSeconds));

        std::string bar = player.isPlaying() ? " >" : " #";
        bar += std::string(barWidth,'=');
        bar = timeLabel + bar;

        move(maxy-1, 1);
        addstr(bar.c_str());
        clrtoeol();
        move(maxy-2, 1);
        addstr(infoLabel.c_str());
    }
    return 0;
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

            getmaxyx(stdscr, win->maxy, win->maxx);

            for (int i = 0; i < win->maxy-4; i++) {
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

            move(win->maxy-3, 1);
            std::string userInputStr = ": " + win->userInput;
            addstr(userInputStr.c_str());
            clrtoeol();

            win->renderBaseUi();

            int k = getch();
            if (k == ERR) {
                // Do nothing!
            } else if (k == 27) { // escape
                win->running = false;
            } else if (k == KEY_DOWN) {
                scrollOffset++;
                scrollOffset = std::clamp(scrollOffset, 0, static_cast<int>(byArtist.size())-win->maxy+4);
                clear();
            } else if (k == KEY_UP) {
                scrollOffset--;
                scrollOffset = std::clamp(scrollOffset, 0, static_cast<int>(byArtist.size())-win->maxy+1);
                clear();
            } else if (k == KEY_RIGHT) {
                if (win->player.isLoaded()) {
                    const double currentPos = win->player.posToSeconds(win->player.getPos());
                    win->player.setPos(win->player.secondsToPos(currentPos+5));
                }
            } else if (k == KEY_LEFT) {
                if (win->player.isLoaded()) {
                    const double currentPos = win->player.posToSeconds(win->player.getPos());
                    win->player.setPos(win->player.secondsToPos(currentPos-5));
                }
            } else if (k == KEY_BACKSPACE) {
                if (!win->userInput.empty()) {
                    win->userInput.pop_back();
                }
            } else if (k == 32) { // space key
                win->player.isPlaying() ? win->player.pause() : win->player.resume();
            } else if (k == KEY_ENTER || k == 10) {
                try {
                    const long trackNumber = stol(win->userInput);
                    try {
                        win->player.stop();
                        // endwin();
                        PlayerActionResult load = win->player.load(byArtist.at(trackNumber)->filePath, true);
                        if (load.result == PlayerActionEnum::PASS) {
                            win->player.setVolume(70);
                            win->player.play();
                            win->currentTrack = byArtist.at(trackNumber);
                        }
                        // initscr();
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

    const int result = menu_handler.call(WindowType::TrackList, this);

    endwin();
    player.stop();
    return result;
}

