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
        auto base_address = button - 16; // 16 is the first dial
        assert(base_address >= 0 && base_address < PHYSICAL_CHANNEL_COUNT);
        m_group.UpdateEncoderFromXT(base_address, attr, false);
    });
    g_xtouch->RegisterFaderCallback([&](unsigned char button, int attr)
    {
        assert(button >= 0 && button <= PHYSICAL_CHANNEL_COUNT);
        if (button < 8) { m_group.UpdateEncoderFromXT(button, attr, true);}
        else { m_group.UpdateMasterEncoder(attr); }
    });
    // g_xtouch->RegisterFaderStateCallback([&](unsigned char fader, int attr)
    // {
    //     assert(fader >= 0 && fader <= PHYSICAL_CHANNEL_COUNT);
    //     bool down = attr == 1;

    //     m_group.FaderTouchState(fader, down); 
    // });

    m_watchDog = std::thread(&XTouchController::WatchDog, this);
    m_watchDog.detach();
    m_group.RegisterMAOutCB([](void*, uint32_t) {});
    m_group.RegisterMaSend(&ma_server);
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

