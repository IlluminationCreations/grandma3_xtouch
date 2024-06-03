#include <Channel.h>
#include <cmath>
#include <string.h>

void Channel::Disable() {
    m_scribblePad.Colour = xt_colours_t::RED;
    g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID - 1, m_scribblePad);
    g_xtouch->SetFaderLevel(PHYSICAL_CHANNEL_ID - 1, 0);
}

void Channel::UpdateEncoderFromXT(int value, bool isFader) {
     if (!m_maServer) {return;}
     if (!isFader) {
         UpdateDial(value);
         return;
     }

    auto address = m_address->Get();
    auto normalized_value = value / 16380.0f; // 0.0f - 1.0f

    IPC::IPCHeader header;
    header.type = IPC::PacketType::UPDATE_MA_ENCODER;
    header.seq = 0; // TODO: Implement sequence number

    IPC::EncoderUpdate::Data packet;
    packet.channel = address.subAddress;
    packet.page = address.mainAddress;
    packet.value = normalized_value * 100.0f;
    packet.encoderType = 200; // Fader

    auto packet_size = sizeof(IPC::IPCHeader) + sizeof(IPC::EncoderUpdate::Data);
    char *buffer = (char*)malloc(packet_size);
    memcpy(buffer, &header, sizeof(IPC::IPCHeader));
    memcpy(buffer + sizeof(IPC::IPCHeader), &packet, sizeof(IPC::EncoderUpdate::Data));
    m_maServer->Send(buffer, packet_size);
    free(buffer);

    m_lastPhysicalChange = std::chrono::system_clock::now();
}

void Channel::UpdateDial(int value) {
     if (!m_maServer) {return;}
    auto address = m_address->Get();
    // 0 = 4xx, 1 = 3xx
    auto &enc = m_toggle ? m_encoders.encoders[1] : m_encoders.encoders[0]; 
    auto current_value = enc.value;

    auto scaled_value = current_value + (1.0f * value); // Value is "relative" from XT (eg (-3,3))
    auto top = fmax(scaled_value, 0.0f);
    auto bottom = fmin(top, 100.0f);
    enc.value = bottom;

    IPC::IPCHeader header;
    header.type = IPC::PacketType::UPDATE_MA_ENCODER;
    header.seq = 0; // TODO: Implement sequence number

    IPC::EncoderUpdate::Data packet;
    packet.channel = address.subAddress;
    packet.page = address.mainAddress;
    packet.value = bottom;
    packet.encoderType = m_toggle ? 300 : 400; 

    auto packet_size = sizeof(IPC::IPCHeader) + sizeof(IPC::EncoderUpdate::Data);
    char *buffer = (char*)malloc(packet_size);
    memcpy(buffer, &header, sizeof(IPC::IPCHeader));
    memcpy(buffer + sizeof(IPC::IPCHeader), &packet, sizeof(IPC::EncoderUpdate::Data));
    m_maServer->Send(buffer, packet_size);
    free(buffer);
}

void Channel::UpdateEncoderFromMA(IPC::PlaybackRefresh::Data encoder) {
    auto address = m_address->Get();
    ASSERT_EQ_INT(address.mainAddress, encoder.page);
    ASSERT_EQ_INT(address.subAddress, encoder.channel);

    // 0,   1,   2
    // 4xx, 3xx, 2xx encoders
    for(int i = 0; i < 3; i++) {
        // Guard Update name
        // Guard Update value
        auto normalized_value = encoder.Encoders[i].value / 100.0f; // 100.f -> (0.0f, 1.0f)

        switch (i) {
            // 4xx (dial lights)
            case 0: {
                auto proportion = round(13 * normalized_value);
                uint32_t integer_value = static_cast<uint32_t>(proportion);
                if (m_encoders.encoders[i].value == encoder.Encoders[i].value) { break; }
                m_encoders.encoders[i].value = encoder.Encoders[i].value;
                g_xtouch->SetDialLevel(PHYSICAL_CHANNEL_ID - 1, integer_value);
                break;
            }
            // 3xx (sound meter lights)
            case 1: {
                auto proportion = round(9 * normalized_value);
                uint32_t integer_value = static_cast<uint32_t>(proportion);
                if (m_encoders.encoders[i].value == encoder.Encoders[i].value) { break; }
                m_encoders.encoders[i].value = encoder.Encoders[i].value;
                g_xtouch->SetMeterLevel(PHYSICAL_CHANNEL_ID - 1, integer_value);
                break;

            }
            // 2xx (fader)
            case 2: {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_lastPhysicalChange);
                if (duration.count() < 500) { break; }
                auto fractional_value = 16380 * normalized_value;
                if (m_encoders.encoders[i].value == fractional_value) { break;  }
                m_encoders.encoders[i].value = fractional_value;


                g_xtouch->SetFaderLevel(PHYSICAL_CHANNEL_ID - 1, fractional_value);
                break;
            }
        }    
    }
}

Channel::Channel(uint32_t id): PHYSICAL_CHANNEL_ID(id) {
    assert(PHYSICAL_CHANNEL_ID >= 1 && PHYSICAL_CHANNEL_ID <= PHYSICAL_CHANNEL_COUNT);
    m_scribblePad = {
        .TopText = {0},
        .BotText = {0},
        .Colour = xt_colours_t::WHITE,
        .Inverted = 1
    };
    m_address = new Observer<Address>({1, id}, [&](Address address) {
        if (address.subAddress == UINT32_MAX) {
            m_scribblePad.Colour = xt_colours_t::BLACK;
            snprintf(m_scribblePad.TopText, 8, "----");
            snprintf(m_scribblePad.BotText, 8, "----");
            g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID - 1, m_scribblePad); // PHYSICAL_CHANNEL_ID is 1-indexed, scribble is 0-indexed
            return;
        }
        m_scribblePad.Colour = xt_colours_t::WHITE;
        snprintf(m_scribblePad.TopText, 8, "%u.%u", address.mainAddress, 100 + address.subAddress);
        snprintf(m_scribblePad.BotText, 8, "%u.%u", address.mainAddress, 100 + address.subAddress);
        g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID - 1, m_scribblePad); // PHYSICAL_CHANNEL_ID is 1-indexed, scribble is 0-indexed
    });
    m_lastPhysicalChange = std::chrono::system_clock::now();
}

void Channel::UpdateScribbleAddress() {
    // g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID, m_scribblePad);
}

void Channel::Pin(bool state) {
    if (m_pinned == state) { return; }

    m_pinned = state;
    if (state) {
        m_scribblePad.Colour = xt_colours_t::PINK;
    } else {
        m_scribblePad.Colour = xt_colours_t::WHITE;
    }
    g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID - 1, m_scribblePad);
}

bool Channel::IsPinned() {
    return m_pinned;
}

void Channel::RegisterMaSend(MaUDPServer *server) {
    m_maServer = server;
}
