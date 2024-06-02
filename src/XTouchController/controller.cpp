#include <XController.h>
#include <chrono>
#include <assert.h>
#include <stdio.h>
#include <set>
#include <memory>
#include <string.h>
#include <guards.h>
#include <cmath>

bool operator<(const Address& a, const Address& b) {
    if (a.mainAddress == b.mainAddress && a.subAddress == b.subAddress) { return false; }

    if (a.mainAddress < b.mainAddress) {
        return true;
    } else if (a.mainAddress == b.mainAddress) {
        return a.subAddress < b.subAddress;
    } else {
        return false;
    }
}

XTouchController::XTouchController() {
    SpawnServer(SERVER_XT);
    assert(g_xtouch != nullptr && "XTouch instance not created");
    assert(g_delayedThreadScheduler != nullptr && "XTouch instance not created");

    g_xtouch->RegisterPacketSender([&](unsigned char *buffer, unsigned int len) 
    {
        assert(xt_server != nullptr && "Server not created");
        xt_server->Send(buffer, len);
    });

    g_xtouch->RegisterButtonCallback([&](unsigned char button, int attr)
    {
        if(!HandleButton(static_cast<xt_buttons>(button), attr == 0)) {
            printf("Button %u hit, state = %u\n", button, attr);
        }
    });
    g_xtouch->RegisterDialCallback([&](unsigned char button, int attr)
    {
        // auto base_address = button - 16; // 16 is the first dial
        // assert(base_address >= 0 && base_address < PHYSICAL_CHANNEL_COUNT);
        // UpdateMaEncoder(button, attr);
    });
    g_xtouch->RegisterFaderCallback([&](unsigned char button, int attr)
    {
        // float adjusted = attr / 16380.0f;
        // printf("Adjusted fader value %f\n", adjusted);
        // g_delayedThreadScheduler->Update(static_cast<RegistrationId>(m_faders[button]), adjusted);
        // printf("Button %u hit, state = %u\n", button, attr);
    });
    g_xtouch->RegisterFaderStateCallback([&](unsigned char button, int attr)
    {
        assert(button >= 0 && button <= PHYSICAL_CHANNEL_COUNT);
        if (button < 8) { UpdateMaEncoder(button, attr);}
        else { UpdateMasterEncoder(attr); }

    });

    m_watchDog = std::thread(&XTouchController::WatchDog, this);
    m_watchDog.detach();
    m_playbackRefresh = std::thread(&XTouchController::RefreshPlaybacks, this);
    m_playbackRefresh.detach();
    m_group.RegisterMAOutCB([](void*, uint32_t) {});
}

void XTouchController::UpdateMaEncoder(uint32_t physical_channel_id, int value) {
    auto addresses = m_group.CurrentChannelAddress();
    auto address = addresses[physical_channel_id];
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
    ma_server.Send(buffer, packet_size);
    free(buffer);
}

void XTouchController::UpdateMasterEncoder(int value) {
    auto normalized_value = value / 16380.0f; // 0.0f - 1.0f

    IPC::IPCHeader header;
    header.type = IPC::PacketType::UPDATE_MA_MASTER;
    header.seq = 0; // TODO: Implement sequence number
    IPC::EncoderUpdate::MasterData packet;
    packet.value = normalized_value * 100.0f;

    auto packet_size = sizeof(IPC::IPCHeader) + sizeof(IPC::EncoderUpdate::Data);
    char *buffer = (char*)malloc(packet_size);
    memcpy(buffer, &header, sizeof(IPC::IPCHeader));
    memcpy(buffer + sizeof(IPC::IPCHeader), &packet, sizeof(IPC::EncoderUpdate::Data));
    ma_server.Send(buffer, packet_size);
    free(buffer);
}

void XTouchController::WatchDog() {
    while(true) {
        if (!xt_server->Alive()) { SpawnServer(SERVER_XT); }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void XTouchController::SpawnServer(SpawnType type) {
    switch (type) {
        case SERVER_XT: {
            if(xt_server != nullptr) { delete xt_server; }
            xt_server = new TCPServer(xt_port, [&] (unsigned char* buffer, uint64_t len)  
                {
                    g_xtouch->HandlePacket(buffer, len);
                }
            );
            break;
        }
        case SERVER_MA: {
            assert(false && "IMPLEMENT");
            break;
        }
        default: {
            assert(false);
        }
    }
}

bool XTouchController::HandleButton(xt_buttons btn, bool down) {
    if (!down) { return true; } // No special handling for button down, yet. Only button up.

    xt_alias_btn btnAlias = static_cast<xt_alias_btn>(btn);
    if (m_group.m_pinConfigMode || btnAlias == xt_alias_btn::PIN) { m_group.UpdatePinnedChannels(btn); return true; }
    if (ButtonUtils::AddressChangingButton(btn)) {  m_group.HandleButtonPress(btnAlias, down); return true;}
    return false;
}



void XTouchController::RefreshPlaybacks() {
    while (true) {
        if (!RefreshPlaybacksImpl()) {
            printf("Failed to refresh playbacks\n");
        };

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}
bool XTouchController::RefreshPlaybacksImpl() {
    uint32_t seq = 0;

    IPC::IPCHeader header;
    header.type = IPC::PacketType::REQ_ENCODERS;
    header.seq = seq;
    IPC::PlaybackRefresh::Request request;
    auto channels = m_group.CurrentChannelAddress();
    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        request.EncoderRequest[i].channel = channels[i].subAddress;
        request.EncoderRequest[i].page = channels[i].mainAddress;
    }

    auto packet_size = sizeof(IPC::IPCHeader) + sizeof(IPC::PlaybackRefresh::Request);
    char *buffer = (char*)malloc(4096);
    memcpy(buffer, &header, sizeof(IPC::IPCHeader));
    memcpy(buffer + sizeof(IPC::IPCHeader), &request, sizeof(IPC::PlaybackRefresh::Request));
    ma_server.Send(buffer, packet_size);

    if (ma_server.Read(buffer, 4096) < 0) {
        printf("Failed to read from MA server\n");
        return false;
    }
    IPC::IPCHeader *resp_header = (IPC::IPCHeader*)buffer;
    if (resp_header->type != IPC::PacketType::RESP_ENCODERS) {
        return false;
    }
    if (resp_header->seq != seq) {
        return false;
    }
    IPC::PlaybackRefresh::ChannelMetadata resp_metadata;
    memcpy(&resp_metadata, buffer + sizeof(IPC::IPCHeader), sizeof(IPC::PlaybackRefresh::ChannelMetadata));
    m_group.UpdateMasterFader(resp_metadata.master);

    IPC::PlaybackRefresh::Data *data = (IPC::PlaybackRefresh::Data*)(buffer);
    for(int i = 0; i < 8; i++) {
        if(!resp_metadata.channelActive[i]) {
            m_group.DisablePhysicalChannel(i);
            continue;
        }
        if (ma_server.Read(buffer, 4096) < 0) {
            printf("Failed to read from MA server\n");
            return false;
        }

        m_group.UpdateEncoderIPC(*data, i);    
    }
    free(buffer);
    return true;
}

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

