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
        if (event.type != PhysicalEventType::BUTTON) { return false; }
        if (event.data.button.Id == xt_alias_btn::CLEAR) {
            bool down = event.data.button.down;
            if (down) {
                g_xtouch->SetSingleButton(xt_alias_btn::CLEAR, xt_button_state_t::ON);
            } else {
                g_xtouch->SetSingleButton(xt_alias_btn::CLEAR, xt_button_state_t::OFF);
            }
            ma_server->SendSystemButton(IPC::ButtonEvent::KeyType::CLEAR, event.data.button.down);
            return true;
        }

        return false;
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
    m_group.RegisterMaSend(&ma_server);
    g_interfaceManager->PushLayer(new ControllerInterfaceLayer(&ma_server));
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
