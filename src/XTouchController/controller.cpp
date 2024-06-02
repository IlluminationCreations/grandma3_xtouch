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
    m_group.RegisterMAOutCB([](void*, uint32_t) {});
    m_group.RegisterMaSend(&ma_server);
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

