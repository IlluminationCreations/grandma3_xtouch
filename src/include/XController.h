#pragma once

#include "x-touch.h"
#include <tcpserver.h>

class XTouchController {
private:
    XTouch xt;
    TCPServer *server;
};