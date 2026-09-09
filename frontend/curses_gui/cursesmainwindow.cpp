// #include "curses_gui/cursesmainwindow.h"
//
// #include <algorithm>
// #include <iomanip>
// #include <iostream>
// #include <ostream>
// #include <string>
// #include <thread>
// #include <locale>
// #include <sstream>
//
// #include "libkoulouri/logger.h"
// #include "libkoulouri/player.h"
//
// CursesMainWindow::CursesMainWindow() {
//     mhandler.populateMetaCache("/home/exii/Music", &mcache);
//     userInput = "";
//     queueIndex = 0;
// };
// CursesMainWindow::~CursesMainWindow() = default;
// void CursesMainWindow::cleanup() {
//     endwin();
// }
//
//
//
//
// void MenuHandler::registerCallback(const WindowType type, const FuncCallback &callback) {
//     // _callbacks.insert(type, callback);
//     // _acallbacks.insert("a", callback);
//     _callbacks[type] = callback;
// }
// int MenuHandler::call(const WindowType type, CursesMainWindow *win, MenuHandler *handler) {
//     return _callbacks[type](win, handler);
// }
//
// void rectangle(int y1, int x1, int y2, int x2)
// {
//     mvhline(y1, x1, 0, x2-x1);
//     mvhline(y2, x1, 0, x2-x1);
//     mvvline(y1, x1, 0, y2-y1);
//     mvvline(y1, x2, 0, y2-y1);
//     mvaddch(y1, x1, ACS_ULCORNER);
//     mvaddch(y2, x1, ACS_LLCORNER);
//     mvaddch(y1, x2, ACS_URCORNER);
//     mvaddch(y2, x2, ACS_LRCORNER);
// }
//
// std::string formatTime(double seconds) {
//     int totalSecs = static_cast<int>(seconds);
//     int hours = totalSecs / 3600;
//     int minutes = (totalSecs % 3600) / 60;
//     int secs = totalSecs % 60;
//
//     std::ostringstream oss;
//     if (hours > 0) {
//         oss << std::setw(2) << std::setfill('0') << hours << ":";
//     }
//     oss << std::setw(2) << std::setfill('0') << minutes << ":"
//         << std::setw(2) << std::setfill('0') << secs;
//     return oss.str();
// }
//
// void CursesMainWindow::handleInternalQueue() {
//     if (player.isCompleted()) {
//         player.stop();
//     }
//
//     if (!player.isLoaded() && queueIndex < queue.size()) {
//         try {
//             const Track* track = queue.at(queueIndex);
//             queueIndex++;
//             player.stop();
//             // endwin();
//             PlayerActionResult load = player.load(track->filePath, true);
//             if (load.result == PlayerActionEnum::PASS) {
//                 player.setVolume(70);
//                 player.play();
//                 currentTrack = track;
//             }
//         } catch (std::out_of_range &e) {
//             queueIndex = 0; // assume queue was cleared, or we've hit the end (++ would put us over)
//         }
//     }
// }
//
// void CursesMainWindow::handleBaseInput(int k) {
//     if (k == ERR) {
//         // Do nothing!
//     } else if (k == 27) {
//         // escape
//         running = false;
//     }
// }
//
// int CursesMainWindow::renderBaseUi(const WindowType winType) {
//     const double positionSeconds = player.posToSeconds(player.getPos());
//     const double maxPositionSeconds = player.posToSeconds(player.getMaxPos());
//
//     // box(stdscr, 0, 0);
//     rectangle(0,0,maxy-2,maxx-1);
//
//     // render basic info
//     std::string title = "[ koulouri - C++ Rewrite";
//     title += " / ";
//     switch (winType) {
//         case WindowType::TrackList: {title += "tracks"; break;}
//         case WindowType::QueueList: {title += "queue"; break;}
//         default: {title += "unknown (report to dev!)"; break;}
//     }
//     title += " / volume: " + std::to_string(player.getVolume()) + " ]";
//     move(0, (maxx/2)-(static_cast<int>(title.length())/2));
//     addstr(title.c_str());
//
//
//     // create song details
//     if (player.isLoaded()) {
//         std::string startTime = formatTime(positionSeconds);
//         std::string endTime = formatTime(maxPositionSeconds);
//         std::string timeLabel = startTime + "-" + endTime;
//         std::string infoLabel = currentTrack->artist + " - " + currentTrack->title;
//
//         const int timeLabelWidth = static_cast<int>(timeLabel.size()) + 1;
//         const int barWidth = static_cast<int>((maxx - timeLabelWidth) * (positionSeconds / maxPositionSeconds));
//
//         std::string bar = player.isPlaying() ? " >" : " #";
//         bar += std::string(barWidth,'=');
//         bar = timeLabel + bar;
//
//         move(maxy-1, 1);
//         addstr(bar.c_str());
//         clrtoeol();
//         move(maxy-2, 1);
//         addstr(infoLabel.c_str());
//     }
//     return 0;
// }
//
// // MENUS
// void trackMenu(CursesMainWindow *win, MenuHandler *handler) {
//     const std::vector<const Track*> byArtist = win->mcache.sortBy([](const Track& a, const Track& b) {
//         return a.artist < b.artist;
//     });
//
//     int scrollOffset = 0;
//     while (win->running) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(20));
//         refresh();
//
//         getmaxyx(stdscr, win->maxy, win->maxx);
//
//         for (int i = 0; i < win->maxy-4; i++) {
//             move(i+1, 1);
//             try {
//                 const Track* track = byArtist.at(i+scrollOffset);
//                 std::string str = std::to_string(i+scrollOffset) + " " + track->artist + " - " + track->title;
//                 addstr(str.c_str());
//                 clrtoeol();
//             } catch (std::out_of_range &e) { // should crash cleanly
//                 endwin();
//                 std::cerr << e.what() << '\n';
//                 initscr();
//                 win->running = false;
//             }
//         }
//
//         move(win->maxy-3, 1);
//         std::string userInputStr = ": " + win->userInput;
//         addstr(userInputStr.c_str());
//         clrtoeol();
//
//         win->renderBaseUi(handler->windowType);
//         win->handleInternalQueue();
//
//         int k = getch();
//         win->handleBaseInput(k);
//         if (k == ERR) {
//             // Do nothing!
//         } else if (k == KEY_DOWN) {
//             scrollOffset++;
//             scrollOffset = std::clamp(scrollOffset, 0, static_cast<int>(byArtist.size())-win->maxy+4);
//             clear();
//         } else if (k == KEY_UP) {
//             scrollOffset--;
//             scrollOffset = std::clamp(scrollOffset, 0, static_cast<int>(byArtist.size())-win->maxy+1);
//             clear();
//         } else if (k == KEY_RIGHT) {
//             if (win->player.isLoaded()) {
//                 const double currentPos = win->player.posToSeconds(win->player.getPos());
//                 win->player.setPos(win->player.secondsToPos(currentPos+5));
//             }
//         } else if (k == KEY_LEFT) {
//             if (win->player.isLoaded()) {
//                 const double currentPos = win->player.posToSeconds(win->player.getPos());
//                 win->player.setPos(win->player.secondsToPos(currentPos-5));
//             }
//         } else if (k == KEY_BACKSPACE) {
//             if (!win->userInput.empty()) {
//                 win->userInput.pop_back();
//             }
//         } else if (k == 32) { // space key
//             win->player.isPlaying() ? win->player.pause() : win->player.resume();
//         } else if (k == KEY_ENTER || k == 10) {
//             try {
//                 const long trackNumber = stol(win->userInput);
//                 try {
//                     win->queue.insert(win->queue.end(), byArtist.at(trackNumber));
//                     // initscr();
//                 } catch (std::out_of_range &e) {
//                     // pass - was out of range
//                 }
//
//                 win->userInput.clear();
//                 clear();
//             } catch (std::invalid_argument &e) {
//                 // pass, no need to do anything here
//             }
//         } else if (isdigit(k)) {
//             win->userInput += static_cast<char>(k);
//         } else if (isascii(k) && k == 'q') {
//             handler->windowType = QueueList;
//             clear();
//             return 0;
//         }
//
//         // refreshes the screen
//         move(0, 0);
//     }
//
//     win->player.stop();
// });
//
// //
//
// int CursesMainWindow::main() {
//     setlocale(LC_ALL, ""); // ensure we can render Unicode
//     initscr();
//     cbreak();
//     noecho();
//     nodelay(stdscr, true);
//     keypad(stdscr, true);
//     UIDispatcher dispatcher;
//     auto menu_handler = MenuHandler();
//     menu_handler.windowType = WindowType::TrackList;
//
//     // MENU DEFINITIONS - BEGIN
//     menu_handler.registerCallback(WindowType::QueueList, [](CursesMainWindow *win, MenuHandler *handler) {
//         int scrollOffset = 0;
//
//         while (win->running) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(20));
//             refresh();
//
//             getmaxyx(stdscr, win->maxy, win->maxx);
//
//             for (int i=0; i < win->maxy-4; i++) {
//                 move(i+1, 1);
//                 try {
//                     const Track* track = win->queue.at(i+scrollOffset);
//                     std::string str = std::to_string(i+scrollOffset) + " " + track->artist + " - " + track->title;
//                     addstr(str.c_str());
//                     clrtoeol();
//                 } catch (std::out_of_range e) {
//                     // pass - queue is likely just too small
//                 }
//             }
//
//             int k = getch();
//
//             win->handleInternalQueue();
//             win->handleBaseInput(k);
//             win->renderBaseUi(handler->windowType);
//         }
//
//         return 0;
//     });
//
//     // MENU DEFINITIONS - END
//
//
//
//     int result = 0;
//     while (running) {
//         result = menu_handler.call(menu_handler.windowType, this, &menu_handler);
//     }
//
//     endwin();
//     player.stop();
//     return result;
// }

#include "curses_gui/cursesmainwindow.h"

UIDispatcher::UIDispatcher() = default;
UIDispatcher::~UIDispatcher() = default;

void UIDispatcher::claimInput(std::thread::id id) {
    std::lock_guard lock(inputMutex);
    claimedThread = id;
}
void UIDispatcher::releaseInput(std::thread::id id) {
    std::lock_guard lock(inputMutex);
    claimedThread = std::thread::id{}; // mark as 'null' via default value
}

int UIDispatcher::getInput(const std::thread::id id) {
    std::lock_guard lock(inputMutex);
    if (claimedThread == id) {
        return lastKey;
    }
    return -1;
}

void UIDispatcher::executeOnMain(const std::function<void(AudioPlayer&, MetaHandler&, MetaCache&)>& task) {
    std::lock_guard lock(queueMutex);
    taskQueue.push(task);
}

void UIDispatcher::drain(AudioPlayer& player, MetaHandler& handler, MetaCache& cache) {
    std::queue<std::function<void(AudioPlayer&, MetaHandler&, MetaCache&)>> localQueue;

    {
        std::lock_guard lock(queueMutex);
        std::swap(localQueue, taskQueue); // reduce lock time
    }

    while (!localQueue.empty()) {
        localQueue.front()(player, handler, cache);
        localQueue.pop();
    }
}


CursesMainWindow::CursesMainWindow() = default;
CursesMainWindow::~CursesMainWindow() = default;

int CursesMainWindow::main() {

    UIDispatcher dispatcher;
    dispatcher.executeOnMain([](AudioPlayer& player, MetaHandler& handler, MetaCache& cache) {
        // handler.populateMetaCache("/home/exii/Music", &cache);
        auto start = std::chrono::high_resolution_clock::now();
        player.load("/home/exii/Music/Hysia/Trauma never cares for casualty/Hysia - Trauma never cares for casualty.flac", true);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << duration.count() << " ms" << std::endl;
        // player.load("/home/exii/Music/Jamie Paige/BIRDBRAIN/Jamie Paige - BIRDBRAIN (with OK Glass).flac", true);
        player.setVolume(70);
        player.play();
    });

    std::cin.get();

    auto start = std::chrono::high_resolution_clock::now();

    // Call the function
    dispatcher.drain(player, mhandler, mcache);

    // Record the end time
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << duration.count() << " ms" << std::endl;

    std::cin.get();
    player.stop();

    for (const Track* track : mcache.sortBy([](const Track& first, const Track& second) {return first.artist > second.artist;})) {
        std::cout << track->artist << " - " << track->title << std::endl;
    }

    return 0;
}
