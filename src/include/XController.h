#pragma once

#include "x-touch.h"
#include <tcpserver.h>
#include <thread>

constexpr unsigned short xt_port = 10111;
constexpr unsigned int PHYSICAL_CHANNEL_COUNT = 8;
// constexpr unsigned int MAX_PAGE_COUNT = 9999;
constexpr unsigned int MAX_PAGE_COUNT = 99; // TODO: Limiting to 99 as the assignment display only has 2 digits. Do we really need 9999 pages?
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

struct Address {
    uint32_t mainAddress;
    uint32_t subAddress;

};
bool operator<(const Address& a, const Address& b);

class AddressObserver {
    Address m_address;
    std::function<void(Address)> m_updateCb;
public:
    AddressObserver(Address address, std::function<void(Address)> updateCb);
    Address Get();
    void Set(Address address);
};

class Channel {
private:
    void UpdateScribbleAddress();

    xt_ScribblePad_t m_scribblePad;
    bool m_pinned = false;

public:
    Channel(uint32_t id);
    void Pin(bool state);
    bool IsPinned();

    const uint32_t PHYSICAL_CHANNEL_ID;
    AddressObserver *m_address; // Virtual address / represents channel within MA
};

class PageObserver {
    uint32_t m_page;
    std::function<void(uint32_t)> m_updateCb;
public:
    PageObserver(uint32_t page, std::function<void(uint32_t)> updateCb);
    uint32_t Get();
    void Set(uint32_t page);
};

class ChannelGroup {
public:
    ChannelGroup();
    void UpdatePinnedChannels(xt_buttons button);
    void ChangePage(int32_t pageOffset); 
    void ScrollPage(int32_t scrollOffset);
    void RegisterMAOutCB(std::function<void(MaIPCPacket&)> requestCb);

    bool m_pinConfigMode = false;
    PageObserver *m_page; // Concrete concept
    uint32_t m_channelOffset = 0; // Offset is relative based on number of channels pinned

private:
    void UpdateWatchList(); 
    void TogglePinConfigMode();
    void GenerateChannelWindows();

    // CBs
    std::function<void(MaIPCPacket&)> cb_RequestMaData;

    // "Other"
    Channel *m_channels;
    std::vector<std::vector<uint32_t>> m_channelWindows;
};

class XTouchController {
public:
    XTouchController();

private:
    struct Address { uint32_t page; uint32_t offset; };
    enum SpawnType { SERVER_XT, SERVER_MA };

    TCPServer *xt_server = nullptr;
    ChannelGroup m_group;
    std::thread m_watchDog;

    void WatchDog();
    void MeterRefresh();
    void SpawnServer(SpawnType type);
    bool HandleButton(xt_buttons btn);
    void HandleAddressChange(xt_alias_btn btn);
};









