#pragma once
#include <ncurses.h>

namespace CursesGui {
    class CursesMainWindow {
    public:
        CursesMainWindow();
        ~CursesMainWindow();

        int main() const;
        static void cleanup();
    private:
        bool running = true;

    };
}

