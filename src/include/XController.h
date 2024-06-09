#pragma once

#include "x-touch.h"
#include <tcpserver.h>
#include <thread>
#include <maserver.h>
#include <string>
#include <delayed.h>
#include <IPC.h>
#include <Channel.h>
#include <ChannelGroup.h>
#include <standard.h>


namespace EncoderType {
    // Ordering is based on the order sent from the LUA plugin (400, 300, 200, 100)
    enum Type { 
        _400 = 0,
        _300 = 1,
        _200 = 2,
        _100 = 3
     }; 
}

class XTouchController {
public:
    XTouchController();

private:
    struct Address { uint32_t page; uint32_t offset; };
    enum SpawnType { SERVER_XT, SERVER_MA };

    TCPServer *xt_server = nullptr;
    MaUDPServer ma_server;
    ChannelGroup *m_group;
    std::thread m_watchDog;

    void WatchDog();
    void SpawnServer(SpawnType type);
    bool HandleButton(xt_buttons btn, bool down);
};









