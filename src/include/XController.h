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

constexpr unsigned short xt_port = 10111;
constexpr unsigned int PHYSICAL_CHANNEL_COUNT = 8;
// constexpr unsigned int MAX_PAGE_COUNT = 9999;
constexpr unsigned int MAX_PAGE_COUNT = 99; // TODO: Limiting to 99 as the assignment display only has 2 digits. Do we really need 9999 pages?
constexpr unsigned int MAX_CHANNEL_COUNT = 90;
enum class ControlType { UNKNOWN, SEGMENT, FADER, KNOB };
enum class SegmentID { UNKNOWN, PAGE };

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
    ChannelGroup m_group;
    std::thread m_watchDog;
    std::thread m_playbackRefresh;

    void WatchDog();
    void SpawnServer(SpawnType type);
    bool HandleButton(xt_buttons btn, bool down);
    
    void RefreshPlaybacks();
    bool RefreshPlaybacksImpl();
    void UpdateMaEncoder(uint32_t physical_channel_id, int value);
    void UpdateMasterEncoder(int value);
};









