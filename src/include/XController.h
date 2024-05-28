#pragma once

#include "x-touch.h"
#include <tcpserver.h>
#include <thread>

constexpr unsigned short xt_port = 10111;
constexpr unsigned int PHYSICAL_CHANNEL_COUNT = 8;
constexpr unsigned int MAX_PAGE_COUNT = 9999;
constexpr unsigned int MAX_CHANNEL_COUNT = 90;
enum class ControlType { UNKNOWN, SEGMENT, FADER, KNOB };
enum class SegmentID { UNKNOWN, PAGE };
namespace IPCMessageType {
    enum RequestType {UNKNOWN, UPDATE_ENCODER_WATCHLIST, SET_PAGE};
}

struct MaIPCPacket {
    IPCMessageType::RequestType type;
    union {
        // IPCMessageType::RequestType::UPDATE_ENCODER_WATCHLIST
        struct {
            unsigned int page;
            unsigned int channel; // eg x01, x02, x03

        } EncoderRequest[8];
        uint32_t page;
    } payload;
};

class XTouchData {
    uint32_t Page;

};

class PlaybackGroup {
    uint32_t m_page;
    uint32_t m_index; // 
    void Update();
};

class ControlState {
private:
    bool m_dirty = true;
    uint32_t m_value;
    ControlType m_type;

public:
    ControlState(ControlType type) : m_type(type) {};
    void Update(uint32_t new_value);
    uint32_t GetValue();
    bool IsDirty();
};

class PageDisplayComponent : public ControlState {
private:
    SegmentID m_segmentId;

public:
    PageDisplayComponent() : ControlState(ControlType::SEGMENT) {};
};

class Dial {

};

class Fader {

};

class Playback {
    uint32_t Page;
    uint32_t Subaddress; // .100
    Dial *Top;
    Dial *Bottom;
    Fader *Fader;

    void RequestUpdate();
};

class Channel {

    bool m_pinned;

    ControlState *m_TopEncoder;
    ControlState *m_BottomEncoder;
    ControlState *m_Fader;
public:
    Channel(uint32_t id);
    void Pin(bool state);
    bool IsPinned();

    const uint32_t PHYSICAL_CHANNEL_ID;
    // Represents the page that contains this executer/playback (eg `1` in `Page 1.102`)
    uint32_t m_mainAddress;
    // Represents the executer within the page (eg `102` in `Page 1.102`)
    uint32_t m_subAddress;
};

class ChannelGroup {
public:
    ChannelGroup();
    void UpdatePinnedChannels(xt_buttons button);
    void ChangePage(int32_t pageOffset); 
    void ScrollPage(int32_t scrollOffset);
    void RegisterMAOutCB(std::function<void(MaIPCPacket&)> requestCb);
    void RegisterButtonLightState(std::function<void(xt_buttons, xt_button_state_t)> requestCb);

    bool m_pinConfigMode;
    uint32_t m_page; // Concrete concept
    uint32_t m_channelOffset; // Offset is relative based on number of channels pinned

private:
    void UpdateWatchList(); 
    void TogglePinConfigMode();

    // Components
    PageDisplayComponent m_pageHardware;

    // CBs
    std::function<void(MaIPCPacket&)> cb_RequestMaData;
    std::function<void(xt_buttons, xt_button_state_t)> cb_SendButtonLightState;

    // "Other"
    Channel *m_channels;
};

class XTouchController {
public:
    XTouchController();

private:
    struct Address { uint32_t page; uint32_t offset; };
    enum SpawnType { SERVER_XT, SERVER_MA };

    XTouch xt;
    TCPServer *xt_server = nullptr;
    ChannelGroup m_group;
    std::thread m_meterThread;
    std::thread m_watchDog;

    PageDisplayComponent m_pageDisplay;

    void WatchDog();
    void MeterRefresh();
    void SpawnServer(SpawnType type);
    bool HandleButton(xt_buttons btn);
    void HandleAddressChange(xt_alias_btn btn);
};









