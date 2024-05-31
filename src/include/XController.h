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

template<typename T>
class Observer {
    T m_page;
    std::function<void(T)> m_updateCb;
public:
    Observer(T page, std::function<void(T)> updateCb) {
        m_updateCb = updateCb;
        Set(page);
    }
    T Get() {
        return m_page;
    }
    void Set(T page) {
        m_page = page;
        m_updateCb(m_page);
    }
};

#define IPC_STRUCT struct __attribute__((__packed__))
namespace IPC {
    namespace PacketType {
        enum Request {UNKNOWN, REQ_ENCODERS};
        enum Response {UNKNOWN, RESPONSE_ENCODERS_METADATA, RESPONSE_ENCODERS_DATA};
    }

    IPC_STRUCT IPCHeader {
        PacketType::Type type;
        uint32_t seq;
    }
    namespace PlaybackRefresh {
        IPC_STRUCT Request {
            IPC_STRUCT {
                unsigned int page;
                unsigned int channel; // eg x01, x02, x03
            } EncoderRequest[8];
        };

        IPC_STRUCT ChannelMetadata{
            float master; // Master fader
            bool channelActive[8]; // True if channel/playback has any active encoders or keys
        }; 

        // Represents the entire column of encoders and keys for a single playback
        IPC_STRUCT Data {
            uint16_t page;
            uint8_t channel; // eg x01, x02, x03
            IPC_STRUCT {
                bool isActive;
                char key_name[8];
                float value;
            } Encoders[3]; // 4xx, 3xx, 2xx encoders
            bool keysActive[4]; // 4xx, 3xx, 2xx, 1xx keys are being used
        };
    }
}



// 

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
    Observer<Address> *m_address; // Virtual address / represents channel within MA
};

class ChannelGroup {
public:
    ChannelGroup();
    void UpdatePinnedChannels(xt_buttons button);
    void ChangePage(int32_t pageOffset); 
    void ScrollPage(int32_t scrollOffset);
    void RegisterMAOutCB(std::function<void(MaIPCPacket&)> requestCb);

    bool m_pinConfigMode = false;
    Observer<uint32_t> *m_page; // Concrete concept
    uint32_t m_channelOffset = 0; // Offset is relative based on number of channels pinned
    uint32_t m_channelOffsetEnd = 0; // Final m_channelWindows index

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









