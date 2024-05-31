#include <cassert>
#include <XController.h>
#include <stdio.h>

XTouch *g_xtouch; 

namespace ChannelGroup_Tests {
    void Helper_PrintPacketEncoderRequest(MaIPCPacket &packet) {
        if(packet.type != IPCMessageType::REQ_ENCODERS) { return; }
        
        for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
            auto channel_data = &packet.data.EncoderRequest[i];
            printf("[%u] Channel=%u, Page=%u\n", i, channel_data->channel, channel_data->page);
        }
        printf("\n");
    }

    void InitialState() {
        printf("-> Running ChannelGroup_Tests::InitialState\n");
        ChannelGroup group;
        assert(group.m_page->Get() == 1);
        assert(group.m_channelOffset == 0);
    }
    void ChangePage() {
        printf("-> Running ChannelGroup_Tests::ChangePage\n");
        ChannelGroup group;
        uint32_t activePage = 1;
        bool handled = false;
        group.RegisterMAOutCB([&](MaIPCPacket &packet) {
            if(packet.type != IPCMessageType::REQ_ENCODERS) {return;}
            for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
                auto channel_data = &packet.data.EncoderRequest[i];
                assert(channel_data->channel == i + 1);
                assert(channel_data->page == activePage);
            }
            handled = true;
        });
        assert(group.m_page->Get() == 1);

        activePage += 1; // Page 2
        group.ChangePage(1);
        assert(handled); handled = false;

        activePage += -1; // Page 1
        group.ChangePage(-1);
        assert(handled); handled = false;

        // Should stay page 1
        group.ChangePage(-1);
        assert(handled == false); 

    }

    void CheckUpdatePinConfigExitButtonLogic() {
        printf("-> Running ChannelGroup_Tests::CheckUpdatePinButtonLogic\n");
        ChannelGroup group;

        // Check our logic for exiting PIN mode
        for(int i = 0; i < xt_buttons::END; i++) {
            auto btn = static_cast<xt_buttons>(i);
            if (btn >= FADER_0_SELECT && btn <= FADER_7_SELECT) { continue; }
            if (btn >= FADER_0_MUTE && btn <= FADER_7_MUTE) { continue; }

            // Enter pin config mode
            group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
            assert(group.m_pinConfigMode);

            // Didn't hit MUTE (for pinned channel), nor SELECT, so we should exit config mode
            group.UpdatePinnedChannels(btn);
            if (group.m_pinConfigMode) {
                printf("btn: %u\n", btn);
                assert(false && "PIN mode was not disabled");
            }
        }

    }
    void CheckPinChangePageLog1() {
        printf("-> Running ChannelGroup_Tests::CheckPinChangePageLog1\n");
        ChannelGroup group;
        // Test pinning first channel. All other channels should be on page 2, from channel 1-7
        bool handled = false;
        group.RegisterMAOutCB([&](MaIPCPacket &packet) {
            if (packet.type != IPCMessageType::REQ_ENCODERS) { return; }
            // Helper_PrintPacketEncoderRequest(packet);
            auto ch_i = 1;
            for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
                auto channel_data = &packet.data.EncoderRequest[i];
                auto channel = channel_data->channel;
                auto page = channel_data->page;
                if (i == 0) { // Physical channel that was pinned
                    assert(page == 1); 
                    assert(channel == 1);
                    continue;
                } 
                if (i == 7) { // Last item in list
                    assert(page == 2); 
                    assert(channel == 7);
                    break;
                } 

                assert(page == 2);
                if (channel != ch_i++) {
                    printf("Expected: %u, Got: %u\n", ch_i-1, channel);
                    Helper_PrintPacketEncoderRequest(packet);
                    assert(false && "Channel did not match expected");
                }
            }
            handled = true;
        });

        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_0_SELECT);
        group.ChangePage(1);
        assert(handled);
    }
    void CheckPinChangePageLog2() {
        printf("-> Running ChannelGroup_Tests::CheckPinChangePageLog2\n");
        ChannelGroup group;
  
        // Test pinning first and last channel. All other channels should be on page 2, from channel 1-6
        bool handled = false;
        group.RegisterMAOutCB([&](MaIPCPacket &packet) {
            if (packet.type != IPCMessageType::REQ_ENCODERS) { return; }

            auto ch_i = 1;
            for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
                auto channel_data = &packet.data.EncoderRequest[i];
                auto channel = channel_data->channel;
                auto page = channel_data->page;
                if (i == 0) { // Physical channel that was pinned
                    assert(page == 1); 
                    assert(channel == 1);
                    continue;
                } 
                if (i == 7) { // Last item in list
                    assert(page == 1); 
                    assert(channel == 8);
                    break;
                } 

                assert(page == 2);
                if (channel != ch_i++) {
                    printf("Expected: %u, Got: %u\n", ch_i-1, channel);
                    Helper_PrintPacketEncoderRequest(packet);
                    assert(false && "Channel did not match expected");
                }
            }
            handled = true;
        });

        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_0_SELECT);
        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_7_SELECT);

        group.ChangePage(1);
        assert(handled);
    }
    void CheckPinChangePageLog3() {
        printf("-> Running ChannelGroup_Tests::CheckPinChangePageLog3\n");
        ChannelGroup group;
  
        // Test pinning first, middle, and last channel. All other channels should be on page 2, from channel 1-5
        bool handled = false;
        group.RegisterMAOutCB([&](MaIPCPacket &packet) {
            if (packet.type != IPCMessageType::REQ_ENCODERS) { return; }

            auto ch_i = 1;
            for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
                auto channel_data = &packet.data.EncoderRequest[i];
                auto channel = channel_data->channel;
                auto page = channel_data->page;
                if (i == 0) { // Physical channel that was pinned
                    assert(page == 1); 
                    assert(channel == 1);
                    continue;
                } 
                if (i == 4) { // Last item in list
                    assert(page == 1); 
                    assert(channel == 5);
                    break;
                } 
                if (i == 7) { // Last item in list
                    assert(page == 1); 
                    assert(channel == 8);
                    break;
                } 

                assert(page == 2);
                if (channel != ch_i++) {
                    printf("Expected: %u, Got: %u\n", ch_i-1, channel);
                    Helper_PrintPacketEncoderRequest(packet);
                    assert(false && "Channel did not match expected");
                }
            }
            handled = true;
        });

        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_0_SELECT);
        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_4_SELECT);
        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_7_SELECT);
        group.ChangePage(1);
        
        assert(handled);
    }
    void CheckPinChangePageLog4() {
        printf("-> Running ChannelGroup_Tests::CheckPinChangePageLog4\n");
        ChannelGroup group;
  
        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_0_SELECT);
        group.ChangePage(1);

        // Test pinning first channel on page 1, last channel on page 2. All other should be on page
        // 3, from 1-6
        bool handled = false;
        group.RegisterMAOutCB([&](MaIPCPacket &packet) {
            if (packet.type != IPCMessageType::REQ_ENCODERS) { return; }

            auto ch_i = 1;
            for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
                auto channel_data = &packet.data.EncoderRequest[i];
                auto channel = channel_data->channel;
                auto page = channel_data->page;
                if (i == 0) { // Physical channel that was pinned
                    assert(page == 1); 
                    assert(channel == 1);
                    continue;
                } 
                if (i == 7) { // Last item in list
                    assert(page == 2); 
                    assert(channel == 7);
                    break;
                } 

                assert(page == 3);
                if (channel != ch_i++) {
                    printf("Expected: %u, Got: %u\n", ch_i-1, channel);
                    Helper_PrintPacketEncoderRequest(packet);
                    assert(false && "Channel did not match expected");
                }
            }
            handled = true;
        });

        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_7_SELECT);
        group.ChangePage(1);
        assert(handled);
    }
    void CheckPinScrollPage() {
        printf("-> Running ChannelGroup_Tests::CheckPinScrollPage\n");
        ChannelGroup group;
  
        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_5_SELECT);

        // Test pinning first channel, then scroll right.
        // There will only be 7 channels available for reassignment,
        // so we expect physical channel 1 to stay on channel 1, then
        // physical channel 2 should be [(width * page_offset) + i]
        // where width = number of physical channels available for reassignment
        bool handled = false;
        group.RegisterMAOutCB([&](MaIPCPacket &packet) {
            if (packet.type != IPCMessageType::REQ_ENCODERS) { return; }

            auto ch_i = 9; 
            for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
                auto channel_data = &packet.data.EncoderRequest[i];
                auto channel = channel_data->channel;
                auto page = channel_data->page;
                if (i == 5) { // Physical channel that was pinned
                    assert(channel == 6);
                    continue;
                } 

                assert(page == 1);
                if (channel != ch_i++) {
                    printf("Expected: %u, Got: %u\n", ch_i-1, channel);
                    Helper_PrintPacketEncoderRequest(packet);
                    assert(false && "Channel did not match expected");
                }
            }
            handled = true;
        });

        group.ScrollPage(1);
        assert(handled);
    }
    void CheckPinChangePageAndScroll() {
        printf("-> Running ChannelGroup_Tests::CheckPinChangePageAndScroll\n");
        ChannelGroup group;
  
        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_0_SELECT);
        group.ChangePage(1);

        // Test pinning first channel, then scroll right.
        // There will only be 7 channels available for reassignment,
        // so we expect physical channel 1 to stay on channel 1, then
        // physical channel 2 should be [(width * page_offset) + i]
        // where width = number of physical channels available for reassignment
        bool handled = false;
        group.RegisterMAOutCB([&](MaIPCPacket &packet) {
            if (packet.type != IPCMessageType::REQ_ENCODERS) { return; }

            auto ch_i = 8; 
            for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
                auto channel_data = &packet.data.EncoderRequest[i];
                auto channel = channel_data->channel;
                auto page = channel_data->page;
                if (i == 0) { // Physical channel that was pinned
                    assert(page == 1); 
                    assert(channel == 1);
                    continue;
                } 

                assert(page == 2);
                if (channel != ch_i++) {
                    printf("Expected: %u, Got: %u\n", ch_i-1, channel);
                    Helper_PrintPacketEncoderRequest(packet);
                    assert(false && "Channel did not match expected");
                }
            }
            handled = true;
        });

        group.ScrollPage(1);
        assert(handled);
    }
    // Check that if we scroll forward, pin a channel, scroll back, then forward again, the pinned channel
    // should not be seen twice.
    void CheckPinNotSeenTwice1() {
        printf("-> Running ChannelGroup_Tests::CheckPinNotSeenTwice1\n");
        ChannelGroup group;

        group.ScrollPage(1);
        group.UpdatePinnedChannels(static_cast<xt_buttons>(xt_alias_btn::PIN));
        group.UpdatePinnedChannels(xt_buttons::FADER_1_SELECT);
        group.ScrollPage(-1);

        // Test pinning first channel, then scroll right.
        // There will only be 7 channels available for reassignment,
        // so we expect physical channel 1 to stay on channel 1, then
        // physical channel 2 should be [(width * page_offset) + i]
        // where width = number of physical channels available for reassignment
        bool handled = false;
        group.RegisterMAOutCB([&](MaIPCPacket &packet) {
            if (packet.type != IPCMessageType::REQ_ENCODERS) { return; }

            for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
                auto channel_data = &packet.data.EncoderRequest[i];
                auto channel = channel_data->channel;
                auto page = channel_data->page;
                if (i == 1) { // Physical channel that was pinned
                    assert(page == 1); 
                    assert(channel == 10);
                    continue;
                } 
                if (i == 7) { // Last item in list
                    assert(channel == 15);
                }

                assert(channel != 10);
            }
            handled = true;
        });
        group.ScrollPage(1);
        assert(handled); handled = false;

        // Fix bug where after fixing issue above, the last channel on the previous scroll page
        // was then assigned to the first channel on the next page.
        group.RegisterMAOutCB([&](MaIPCPacket &packet) {
            if (packet.type != IPCMessageType::REQ_ENCODERS) { return; }

            for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
                auto channel_data = &packet.data.EncoderRequest[i];
                auto channel = channel_data->channel;
                auto page = channel_data->page;
                if (i == 0) { // Physical channel that was pinned
                    if (channel == 15) {
                        Helper_PrintPacketEncoderRequest(packet);
                        assert(false && "Channel did not match expected");
                    }
                    continue;
                } 
            }
            handled = true;
        });
        group.ScrollPage(1);
        assert(handled);


    }
}

int main(int, char**) {
    g_xtouch = new XTouch();

    printf("------------ Running ChannelGroup tests ------------ \n");
    ChannelGroup_Tests::InitialState();
    ChannelGroup_Tests::ChangePage();
    ChannelGroup_Tests::CheckUpdatePinConfigExitButtonLogic();
    ChannelGroup_Tests::CheckPinChangePageLog1();
    ChannelGroup_Tests::CheckPinChangePageLog2();
    ChannelGroup_Tests::CheckPinChangePageLog3();
    ChannelGroup_Tests::CheckPinChangePageLog4();
    ChannelGroup_Tests::CheckPinScrollPage();
    ChannelGroup_Tests::CheckPinChangePageAndScroll();
    ChannelGroup_Tests::CheckPinNotSeenTwice1();
    // ChannelGroup_Tests::CheckPinScrollForwardTestBack();

    printf("---------------- TESTING COMPLETE ----------------\n");

    return 0;
}