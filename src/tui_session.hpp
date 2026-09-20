
#pragma once

#include "utils.hpp"
#include <vector>

class ITUISessionListener
{
  public:
    virtual void onUpdate() = 0;
    virtual void onTerminalSizeChange(Rectangle newSize) = 0;
};

class TUISession
{
  public:
    TUISession();
    ~TUISession();

    void run();

    void addTUISessionListener(ITUISessionListener *listener);
    void removeTUISessionListener(ITUISessionListener *listener);

  private:
    void onTerminalSizeChange();
    void updatePresentationWindow();

    Rectangle _currentTermSize;
    std::vector<ITUISessionListener *> _listeners;
};
