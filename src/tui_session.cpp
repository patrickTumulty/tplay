
#include "tui_session.hpp"
#include <algorithm>
#include <memory>
#include <thread>

TUISession::TUISession()
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
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
    for (auto listener : _listeners)
        listener->onTerminalSizeChange(_currentTermSize);
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

void TUISession::addTUISessionListener(std::shared_ptr<ITUISessionListener> listener)
{
    _listeners.push_back(listener);
}

void TUISession::removeTUISessionListener(std::shared_ptr<ITUISessionListener> listener)
{
    _listeners.erase(std::remove(_listeners.begin(), _listeners.end(), listener), _listeners.end());
}

void TUISession::run()
{
    while (true)
    {
        clear();

        for (auto listener : _listeners)
            listener->onTerminalUpdate();

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

        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}
