
#include "tui_session.hpp"

TUISession::TUISession()
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    // nodelay(stdscr, TRUE);
    curs_set(0);

    getmaxyx(stdscr, _currentTermSize.height, _currentTermSize.width);
}

TUISession::~TUISession()
{
    endwin();
}

void TUISession::onTerminalSizeChange()
{
    getmaxyx(stdscr, _currentTermSize.height, _currentTermSize.width);
}

void TUISession::updatePresentationWindow()
{
    Rectangle videoFrame = _currentTermSize;
    videoFrame.height -= 1;
    videoFrame.width -= 1;

    auto rec = fitDimensionsToRatio(videoFrame, 32.0f / 9.0f);
    int offsetX = std::max(1, (videoFrame.width - rec.width) / 2);
    int offsetY = std::max(0, (videoFrame.height - rec.height) / 2);

    drawBox(offsetX, offsetY, rec.height, rec.width);
}

void TUISession::run()
{
    while (true)
    {
        clear();

        updatePresentationWindow();

        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE)
        {
            onTerminalSizeChange();
        }
        else if (ch == 27) // ESC
        {
            break;
        }
    }
}
