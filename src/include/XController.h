#pragma once

#include "x-touch.h"
#include <tcpserver.h>

constexpr unsigned short xt_port = 10111;

class XTouchController {
private:
    enum SpawnType { SERVER_XT, SERVER_MA };
    XTouch xt;
    TCPServer *xt_server = nullptr;

    void WatchDog();
    void SpawnServer(SpawnType type);

public:
    XTouchController();
};