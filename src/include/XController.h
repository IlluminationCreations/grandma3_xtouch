#pragma once

#include "x-touch.h"
#include <tcpserver.h>
#include <thread>
#include <maserver.h>
#include <string>
#include <delayed.h>
#include <IPC.h>

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

namespace EncoderType {
    // Ordering is based on the order sent from the LUA plugin (400, 300, 200, 100)
    enum Type { 
        _400 = 0,
        _300 = 1,
        _200 = 2,
        _100 = 3
     }; 
}

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
    struct EncoderColumn {
        struct {
            bool active;
            std::string name;
            float value;
        } encoders[3]; // 4xx, 3xx, 2xx encoders
        bool keysActive[4]; // 4xx, 3xx, 2xx, 1xx keys are being used
    }; // Slimmed down version of IPC::PlaybackRefresh::Data
    void UpdateScribbleAddress();

    xt_ScribblePad_t m_scribblePad;
    bool m_pinned = false;
    EncoderColumn m_encoders;

public:
    Channel(uint32_t id);
    void UpdateEncoderIPC(IPC::PlaybackRefresh::Data encoder);
    void Pin(bool state);
    bool IsPinned();
    void Disable();

    const uint32_t PHYSICAL_CHANNEL_ID;
    Observer<Address> *m_address; // Virtual address / represents channel within MA
};

class ChannelGroup {
public:
    ChannelGroup();
    void UpdateFader(uint32_t channel, float value);
    void UpdatePinnedChannels(xt_buttons button);
    void ChangePage(int32_t pageOffset); 
    void ScrollPage(int32_t scrollOffset);
    void RegisterMAOutCB(std::function<void(char*, uint32_t)> requestCb);
    std::vector<Address> CurrentChannelAddress();
    void UpdateEncoderIPC(IPC::PlaybackRefresh::Data encoder, uint32_t physical_channel_id);
    void UpdateMasterFader(float value);
    void DisablePhysicalChannel(uint32_t channel);

    bool m_pinConfigMode = false;
    Observer<uint32_t> *m_page; // Concrete concept
    uint32_t m_channelOffset = 0; // Offset is relative based on number of channels pinned
    uint32_t m_channelOffsetEnd = 0; // Final m_channelWindows index
    float m_masterFader = 0.0f;


private:
    void UpdateWatchList(); 
    void TogglePinConfigMode();
    void GenerateChannelWindows();

    // CBs
    std::function<void(char*, uint32_t)> cb_RequestMaData;

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
    MaUDPServer ma_server;
    ChannelGroup m_group;
    std::thread m_watchDog;
    std::thread m_playbackRefresh;

    void WatchDog();
    void SpawnServer(SpawnType type);
    bool HandleButton(xt_buttons btn);
    void HandleAddressChange(xt_alias_btn btn);
    void RefreshPlaybacks();
    bool RefreshPlaybacksImpl();
    void UpdateMaEncoder(uint32_t physical_channel_id, int value);
    void UpdateMasterEncoder(int value);
};









