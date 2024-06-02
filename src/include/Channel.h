#pragma once

#include <Observer.h>
#include <Address.h>

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
