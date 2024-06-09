#include <XController.h>
#include <chrono>
#include <assert.h>
#include <stdio.h>
#include <set>
#include <memory>
#include <string.h>
#include <guards.h>
#include <cmath>

class ControllerInterfaceLayer : public InterfaceLayer {
    MaUDPServer *ma_server;
public:
    ControllerInterfaceLayer(MaUDPServer *server) : ma_server(server) {}
    void Resume() override {}
    void Pause() override {}
    void Start() override {}
    void Removed() override {
        assert(false); // Should never be removed
    }
    bool HandleInput(PhysicalEvent event) override {
        // TODO: Fix this mess, for sure.
        if (event.type != PhysicalEventType::BUTTON) { return false; }
        if (
            event.data.button.Id != xt_alias_btn::CLEAR &&
            event.data.button.Id != xt_alias_btn::STORE &&
            event.data.button.Id != xt_alias_btn::UPDATE &&
            event.data.button.Id != xt_alias_btn::ASSIGN &&
            event.data.button.Id != xt_alias_btn::MOVE &&
            event.data.button.Id != xt_alias_btn::OOPS &&
            event.data.button.Id != xt_alias_btn::EDIT
        ) { return false; }

        bool down = event.data.button.down;
        if (down) {
            g_xtouch->SetSingleButton(event.data.button.Id, xt_button_state_t::ON);
        } else {
            g_xtouch->SetSingleButton(event.data.button.Id, xt_button_state_t::OFF);
        }
        if (event.data.button.Id == xt_alias_btn::CLEAR) {
            ma_server->SendSystemButton(IPC::ButtonEvent::KeyType::CLEAR, event.data.button.down);
        }
        if (event.data.button.Id == xt_alias_btn::STORE) {
            ma_server->SendSystemButton(IPC::ButtonEvent::KeyType::STORE, event.data.button.down);
        }
        if (event.data.button.Id == xt_alias_btn::UPDATE) {
            ma_server->SendSystemButton(IPC::ButtonEvent::KeyType::UPDATE, event.data.button.down);
        }
        if (event.data.button.Id == xt_alias_btn::ASSIGN) {
            ma_server->SendSystemButton(IPC::ButtonEvent::KeyType::ASSIGN, event.data.button.down);
        }
        if (event.data.button.Id == xt_alias_btn::MOVE) {
            ma_server->SendSystemButton(IPC::ButtonEvent::KeyType::MOVE, event.data.button.down);
        }
        if (event.data.button.Id == xt_alias_btn::OOPS) {
            ma_server->SendSystemButton(IPC::ButtonEvent::KeyType::OOPS, event.data.button.down);
        }
        if (event.data.button.Id == xt_alias_btn::EDIT) {
            ma_server->SendSystemButton(IPC::ButtonEvent::KeyType::EDIT, event.data.button.down);
        }

        return true;
    }
};

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

    m_watchDog = std::thread(&XTouchController::WatchDog, this);
    m_watchDog.detach();

    // Needs to be pushed before the ChannelGroup is created
    g_interfaceManager->PushLayer(new ControllerInterfaceLayer(&ma_server));

    m_group = new ChannelGroup();
    m_group->RegisterMaSend(&ma_server);
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
