#include "curses_gui/cursesmainwindow.h"

#include <string>
#include <thread>

using namespace CursesGui;

CursesMainWindow::CursesMainWindow() = default;
CursesMainWindow::~CursesMainWindow() = default;

void CursesMainWindow::cleanup() {
    endwin();
}


int CursesMainWindow::main() const {

    // init screen and sets up screen
    initscr();

    int i = 0;

    while (running) {
        i += 1;
        // print to screen
        std::string msg = "Hello, world!" + std::to_string(i);
        printw("%s", msg.c_str());

        int k = getch();
        if (k) {
            printw("%d", k);
        }

        // refreshes the screen
        refresh();
        clear();
        move(0, 0);

        std::this_thread::sleep_for(std::chrono::microseconds(5000));
    }
    // deallocates memory and ends ncurses
    endwin();
    return 0;
}

