#include <Channel.h>
#include <cmath>

void Channel::Disable() {
    m_scribblePad.Colour = xt_colours_t::RED;
    g_xtouch->SetScribble(PHYSICAL_CHANNEL_ID - 1, m_scribblePad);
    g_xtouch->SetFaderLevel(PHYSICAL_CHANNEL_ID - 1, 0);
}

void Channel::UpdateEncoderIPC(IPC::PlaybackRefresh::Data encoder) {
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
                m_encoders.encoders[i].value = integer_value;
                g_xtouch->SetDialLevel(PHYSICAL_CHANNEL_ID - 1, integer_value);
                break;
            }
            // 3xx (sound meter lights)
            case 1: {
                auto proportion = round(9 * normalized_value);
                uint32_t integer_value = static_cast<uint32_t>(proportion);
                m_encoders.encoders[i].value = integer_value;
                g_xtouch->SetMeterLevel(PHYSICAL_CHANNEL_ID - 1, integer_value);
                break;

            }
            // 2xx (fader)
            case 2: {
                auto fractional_value = 16380 * normalized_value;
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

