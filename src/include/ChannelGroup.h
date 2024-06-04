#pragma once
#include <standard.h>
#include <Observer.h>
#include <Address.h>
#include <vector>
#include <functional>
#include <Channel.h>
#include <maserver.h>
#include <mutex>
#include <delayed.h>
#include <chrono>

enum class UpdateType {
    FADER,
    DIAL,
    BUTTON,
    MASTER
};

class ChannelGroup {
public:
    using clock = std::chrono::high_resolution_clock;
    using time_point = std::chrono::time_point<clock>;
    ChannelGroup();
    void UpdateFader(uint32_t channel, float value);
    void UpdatePinnedChannels(xt_buttons button);
    void ChangePage(int32_t pageOffset); 
    void ScrollPage(int32_t scrollOffset);
    void RegisterMAOutCB(std::function<void(char*, uint32_t)> requestCb);
    void RegisterMaSend(MaUDPServer *server); // Temporary, will be removed after refactoring
    std::vector<Address> CurrentChannelAddress();
    void UpdateEncoderFromMA(IPC::PlaybackRefresh::Data encoder, uint32_t physical_channel_id);
    void DisablePhysicalChannel(uint32_t channel);

    void HandleUpdate(UpdateType type, char button, int value);
    void HandleFaderUpdate(char button, int value);
    void HandleDialUpdate(char button, int value);
    void HandleButtonPress(char button, bool down);

    void UpdateEncoderFromXT(uint32_t physical_channel_id, int value, bool isFader);
    void UpdateMasterEncoder(int value);

    bool m_pinConfigMode = false;
    Observer<uint32_t> *m_page; // Concrete concept

    uint32_t m_channelOffset = 0; // Offset is relative based on number of channels pinned
    uint32_t m_channelOffsetEnd = 0; // Final m_channelWindows index
    float m_masterFader = 0.0f;
    uint32_t m_sequence = 0;

private:

    void UpdateWatchList(); 
    void TogglePinConfigMode();
    void GenerateChannelWindows();
    void HandleAddressChange(xt_alias_btn btn);
    void RefreshPlaybacks();
    bool RefreshPlaybacksImpl();

    // CBs
    std::function<void(char*, uint32_t)> cb_RequestMaData;
    MaUDPServer *m_maServer;
    std::function<void(char*, uint32_t)> cb_Send; // Temporary, will be removed after refactoring

    // "Other"
    Channel *m_channels;
    Encoder *m_masterFaderEncoder;
    std::vector<std::vector<uint32_t>> m_channelWindows;
    std::thread m_playbackRefresh;
};