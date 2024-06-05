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
    auto &enc = m_encoder[2]; // Fader

    auto address = m_address->Get();
    auto normalized_value = (value / 16380.0f) * 100.0f; // 0.0f - 100.0f
    enc.SetValue(normalized_value, true);

    IPC::IPCHeader header;
    header.type = IPC::PacketType::UPDATE_MA_ENCODER;
    header.seq = 0; // TODO: Implement sequence number

    IPC::EncoderUpdate::Data packet;
    packet.channel = address.subAddress;
    packet.page = address.mainAddress;
    packet.value = normalized_value;
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
    auto &enc = m_toggle ? m_encoder[1] : m_encoder[0]; 
    auto current_value = enc.GetValue();


    auto func = [](int x) -> float {
        bool negative = x < 0;
        auto _pow = pow(abs(x), 1.2);
        auto result = (0.665 * _pow);
        result = negative ? -result : result;
        return result;
    };

    auto scaled_value = current_value + func(value); // Value is "relative" from XT (eg (-3,3))
    auto top = fmax(scaled_value, 0.0f);
    auto bottom = fmin(top, 100.0f);
    enc.SetValue(bottom, true);

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

    m_lastPhysicalChange = std::chrono::system_clock::now();
}

void Channel::UpdateEncoderFromMA(IPC::PlaybackRefresh::Data encoder) {
    auto address = m_address->Get();
    ASSERT_EQ_INT(address.mainAddress, encoder.page);
    ASSERT_EQ_INT(address.subAddress, encoder.channel);


    // 0,   1,   2
    // 4xx, 3xx, 2xx encoders
    for(int i = 0; i < 3; i++) {
        m_encoder[i].SetValue(encoder.Encoders[i].value, false);   
        m_encoder[i].SetActive(encoder.Encoders[i].isActive);

        // Code below is for encoders 4xx and 3xx       
        if (i == 3) { continue; }

        bool nameWasSet = m_encoder[i].SetName(encoder.Encoders[i].key_name);
        if (!nameWasSet) { continue; } // If the name was not set, don't update the scribble pad
        if (m_toggle && i == 1) // m_toggle means we're displaying the 3xx encoder
        {

            snprintf(m_scribblePad.BotText, 8, "%s", encoder.Encoders[1].key_name);
            g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID - 1, m_scribblePad);
        }
        else if (!m_toggle && i == 0) 
        {
            snprintf(m_scribblePad.BotText, 8, "%s", encoder.Encoders[0].key_name);
            g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID - 1, m_scribblePad);
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
        // snprintf(m_scribblePad.BotText, 8, "%u.%u", address.mainAddress, 100 + address.subAddress);
        g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID - 1, m_scribblePad); // PHYSICAL_CHANNEL_ID is 1-indexed, scribble is 0-indexed
    });
    m_lastPhysicalChange = std::chrono::system_clock::now();


    m_encoder = (Encoder*)(malloc(sizeof(Encoder) * 3));
    for(int i = 0; i < 3; i++) {
        auto channel = new (&m_encoder[i]) Encoder(static_cast<EncoderId>(i), PHYSICAL_CHANNEL_ID);
    }
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

Encoder::Encoder(EncoderId type, uint32_t id): m_type(type), PHYSICAL_CHANNEL_ID(id) {
    m_lastPhysicalChange = std::chrono::system_clock::now(); // Initialize to now

    // Spent a lot of time trying to figure out why there was stuttering on the dials
    // The 'stuttering' led to the dial, when being turned, would periodically 'jump' back to the previous value
    // After much diagnosis, the issue is that when calling SetFader in GrandMA, the value is updated for the next DMX packet
    // This means that the value is not immediately updated, and a request for encoder states would return the current value, not the updated value
    // This was confirmed by printing the update value before calling SetFader, then printing the output of GetFader({}).
    // The result was that the value was not updated, and the value was the same as the previous value.
    if (type == EncoderId::Fader || type == EncoderId::Master) {
        m_delayTime = 500;
    } else {
        m_delayTime = 50;
    }
}

float Encoder::GetValue() {
    return m_value;
}
void Encoder::SetValue(float value, bool physical) {
    if (!physical)
    { 
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_lastPhysicalChange); 
        if (duration.count() < m_delayTime ) { return; }
    } 
    else 
    {
        m_lastPhysicalChange = std::chrono::system_clock::now();
    }


    if (m_value == value) { return; }
    m_value = value;
    switch (m_type) {
        case EncoderId::Dial: 
        {
            auto proportion = round(13 * (value / 100.0f));
            uint32_t integer_value = static_cast<uint32_t>(proportion);
            g_xtouch->SetDialLevel(PHYSICAL_CHANNEL_ID - 1, integer_value);
            break;
        }
        case EncoderId::SoundMeter: 
        {
            auto proportion = round(9 * (value / 100.0f));
            uint32_t integer_value = static_cast<uint32_t>(proportion);
            g_xtouch->SetMeterLevel(PHYSICAL_CHANNEL_ID - 1, integer_value);
            break;
        }
        case EncoderId::Fader: 
        {
            if (physical) { break; } // Don't need to update fader when it's physical
            auto fractional_value = 16380 * (value / 100.0f);
            m_value = fractional_value;
            g_xtouch->SetFaderLevel(PHYSICAL_CHANNEL_ID - 1, fractional_value);
            break;
        }
        case EncoderId::Master: 
        {
            if (physical) { break; } // Don't need to update fader when it's physical
            auto fractional_value = 16380 * (value / 100.0f);
            m_value = fractional_value;
            g_xtouch->SetFaderLevel(8, fractional_value);
            break;
        }
        default: 
        {
            assert(false);
        }
    }
    m_value = value;
}
void Channel::Toggle() {
    // We don't need to toggle if an alternative encoder is not active
    if (!m_encoder[0].IsActive() || !m_encoder[1].IsActive()) { return; } 
    m_toggle = !m_toggle;
    std::string text;
    if (!m_toggle) { // 400 text
        text = m_encoder[0].GetName();
    } else { // 300 text
        text = m_encoder[1].GetName();
    }
    snprintf(m_scribblePad.BotText, 8, "%s", text.c_str());
    g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID - 1, m_scribblePad);
}
bool Encoder::SetName(std::string name) {
    if (name == m_name) { return false; }
    m_name = name;
    return true;

}
std::string Encoder::GetName() {
    return m_name;
}
void Encoder::SetActive(bool state) {
    m_active = state;
}
bool Encoder::IsActive() {
    return m_active;
}
