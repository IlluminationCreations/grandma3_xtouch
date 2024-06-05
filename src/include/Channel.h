#pragma once
#include <standard.h>
#include <Observer.h>
#include <Address.h>
#include <x-touch.h>
#include <string>
#include <IPC.h>
#include <guards.h>
#include <maserver.h>
#include <chrono>

enum class EncoderId {
    Dial,
    SoundMeter,
    Fader,
    Master // Special case for master fader, only contained in ChannelGroup
};

class Encoder {
private:
    using clock = std::chrono::system_clock;
    using time_point = std::chrono::time_point<clock>;

    EncoderId m_type; 
    float m_value;
    bool m_active = false;
    std::string m_name;
    const uint32_t PHYSICAL_CHANNEL_ID;
    time_point m_lastPhysicalChange;
    uint32_t m_delayTime;

public:
    Encoder(EncoderId type, uint32_t id);
    float GetValue();
    void SetValue(float value, bool physical);
    bool SetName(std::string name);
    std::string GetName();
    void SetActive(bool state);
    bool IsActive();
};

class Channel {
private:
    void UpdateScribbleAddress();

    xt_ScribblePad_t m_scribblePad;
    bool m_pinned = false;
    bool m_firstUpdateReceived = false;
    Encoder *m_encoder;
    bool m_keysActive[4];
    MaUDPServer *m_maServer;
    bool m_toggle = false;
    void UpdateDial(int value);
    std::chrono::time_point<std::chrono::system_clock> m_lastPhysicalChange;

public:
    Channel(uint32_t id);
    // Updates internal value, and sends data to GrandMA3
    void UpdateEncoderFromXT(int value, bool isFader);
    // Updates value and fader based on GrandMA3 state
    void UpdateEncoderFromMA(IPC::PlaybackRefresh::Data encoder);
    void Pin(bool state);
    bool IsPinned();
    void Disable();
    void RegisterMaSend(MaUDPServer *server);
    void Toggle();

    const uint32_t PHYSICAL_CHANNEL_ID;
    Observer<Address> *m_address; // Virtual address / represents channel within MA
};
