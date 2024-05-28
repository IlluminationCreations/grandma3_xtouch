#include <XController.h>
#include <thread>
#include <chrono>
#include <assert.h>
#include <stdio.h>

XTouchController::XTouchController() {
    xt.RegisterPacketSender([&](unsigned char *buffer, unsigned int len) 
    {
        xt_server->Send(buffer, len);
    });

    xt.RegisterButtonCallback([&](unsigned char button, unsigned int len)
    {
        printf("Button %u hit, state = %u\n", button, len);
    });

    SpawnServer(SERVER_XT);
}

void XTouchController::WatchDog() {
    while(true) {
        if (!xt_server->Alive()) { SpawnServer(SERVER_XT); }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

void XTouchController::SpawnServer(SpawnType type) {
    switch (type) {
        case SERVER_XT: {
            if(xt_server != nullptr) { delete xt_server; }
            xt_server = new TCPServer(xt_port, [&] (unsigned char* buffer, uint64_t len)  
                {
                    xt.HandlePacket(buffer, len);
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