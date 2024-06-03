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

class Channel {
private:
    struct EncoderColumn {
        // 0 = 4xx, 1 = 3xx, 2 = 2xx
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
    MaUDPServer *m_maServer;
    bool m_toggle = false;
    void UpdateDial(int value);

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

    const uint32_t PHYSICAL_CHANNEL_ID;
    Observer<Address> *m_address; // Virtual address / represents channel within MA
};
