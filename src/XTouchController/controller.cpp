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

    // g_xtouch->RegisterButtonCallback([&](unsigned char button, int attr)
    // {
    //     if(!HandleButton(static_cast<xt_buttons>(button), attr == 1)) {
    //         printf("Button %u hit, state = %u\n", button, attr);
    //     }
    // });
    // g_xtouch->RegisterDialCallback([&](unsigned char button, int attr)
    // {
    //     // Addresses of the dials are 16-23
    //     if (button >= 16 && button <= 23) {
    //         m_group.HandleUpdate(UpdateType::DIAL, button - 16, attr);
    //         return;
    //     }
    //     printf("Dial %u hit, state = %u\n", button, attr);
    // });
    // g_xtouch->RegisterFaderCallback([&](unsigned char button, int attr)
    // {
    //     assert(button >= 0 && button <= PHYSICAL_CHANNEL_COUNT);
    //     if (button < 8) 
    //     {
    //         m_group.HandleUpdate(UpdateType::FADER, button, attr);
    //     }
    //     else 
    //     {
    //         m_group.HandleUpdate(UpdateType::MASTER, 0, attr);
    //     }
    // });

    m_watchDog = std::thread(&XTouchController::WatchDog, this);
    m_watchDog.detach();
    m_group.RegisterMaSend(&ma_server);
}

void XTouchController::WatchDog() {
    while(true) {
        if (!xt_server->Alive()) { assert(false); SpawnServer(SERVER_XT); }
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
        xt_alias_btn btnAlias = static_cast<xt_alias_btn>(btn);
    if (m_group.InPinMode() || btnAlias == xt_alias_btn::PIN) { m_group.UpdatePinnedChannels(btn, down); return true; }
    if (ButtonUtils::AddressChangingButton(btn)) {  m_group.HandleButtonPress(btnAlias, down); return true;}
    if (btn >= FADER_0_DIAL_PRESS && btn <= FADER_7_DIAL_PRESS) { m_group.HandleButtonPress(btn, down); return true; }
    if (btn >= FADER_0_REC && btn <= FADER_7_SELECT) { m_group.HandleButtonPress(btn, down); return true; }
    return false;
}

