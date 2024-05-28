#include <XController.h>
#include <chrono>
#include <assert.h>
#include <stdio.h>

XTouchController::XTouchController() {
    xt.RegisterPacketSender([&](unsigned char *buffer, unsigned int len) 
    {
        xt_server->Send(buffer, len);
    });

    xt.RegisterButtonCallback([&](unsigned char button, int attr)
    {
        if (attr == 0) { return; } // No special handling for button up, yet.

        if(!HandleButton(static_cast<xt_buttons>(button))) {
            printf("Button %u hit, state = %u\n", button, attr);
        }
    });
    xt.RegisterDialCallback([&](unsigned char button, int attr)
    {
        printf("Button %u hit, szate = %d\n", button, attr);
    });
    xt.RegisterFaderCallback([&](unsigned char button, int attr)
    {
        printf("Button %u hit, state = %u\n", button, attr);
    });
    xt.RegisterFaderStateCallback([&](unsigned char button, int attr)
    {
        printf("Button %u hit, state = %u\n", button, attr);
    });

    SpawnServer(SERVER_XT);

    m_meterThread = std::thread(&XTouchController::MeterRefresh, this);
    m_watchDog = std::thread(&XTouchController::WatchDog, this);
    m_group.RegisterMAOutCB([](MaIPCPacket &packet) {});
    m_group.RegisterButtonLightState([&](xt_buttons btn, xt_button_state_t state) {
        xt.SetSingleButton(btn, state);
    });
}

void XTouchController::WatchDog() {
    while(true) {
        if (!xt_server->Alive()) { SpawnServer(SERVER_XT); }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

void XTouchController::MeterRefresh() {
    while(true) {
        xt.SendAllMeters();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

void PlaybackGroup::Update() {

}

void ControlState::Update(uint32_t value) {
    if (value == m_value) { return; }

    m_value = value;
    m_dirty = true;
}

bool ControlState::IsDirty() {
    return m_dirty;
}

uint32_t ControlState::GetValue() {
    return m_value;
}

bool XTouchController::HandleButton(xt_buttons btn) {
    xt_alias_btn btnAlias = static_cast<xt_alias_btn>(btn);
    if (m_group.m_pinConfigMode || btnAlias == xt_alias_btn::PIN) { m_group.UpdatePinnedChannels(btn); return true; }
    if (ButtonUtils::AddressChangingButton(btn)) { HandleAddressChange(btnAlias); return true; }
    return false;
}

void XTouchController::HandleAddressChange(xt_alias_btn btn) {
    switch (btn) {
        case xt_alias_btn::EXECUTER_SCROLL_LEFT: 
        case xt_alias_btn::EXECUTER_SCROLL_RIGHT: 
        {
            uint32_t offset = btn == xt_alias_btn::EXECUTER_SCROLL_LEFT ? -1 : 1;
            m_group.ScrollPage(offset);
            return;
        }
        case xt_alias_btn::PAGE_DEC:
        case xt_alias_btn::PAGE_INC: 
        {
            uint32_t offset = btn == xt_alias_btn::PAGE_DEC ? -1 : 1; 
            m_group.ChangePage(offset);
            m_pageDisplay.Update(m_group.m_page);
            return;
        }
        default: { assert(false); }
    }
}

void ChannelGroup::UpdatePinnedChannels(xt_buttons button) {
    int selectBtnToChannel = ButtonUtils::SelectButtonToChannel(button);
    int muteBtnToChannel = ButtonUtils::MuteButtonToChannel(button);
    bool btnOutsideRange = 
        (selectBtnToChannel == -1) && 
        (muteBtnToChannel == -1) && 
        (button != xt_alias_btn::PIN);
    bool muteButtonHit = muteBtnToChannel != -1;
    bool selectButtonHit = selectBtnToChannel != -1;

    if (m_pinConfigMode) {
        TogglePinConfigMode();
        if (btnOutsideRange) {
            return;
        } 
        if (muteButtonHit) {
            printf("Unpinning channel %u\n", muteBtnToChannel);
            m_channels[muteBtnToChannel].Pin(false);
            return;
        }
        if (selectButtonHit) {
            printf("Pinning channel %u\n", selectBtnToChannel);
            m_channels[selectBtnToChannel].Pin(true);
            return;
        }
        return;
    } 
    if (button == xt_alias_btn::PIN) {
        TogglePinConfigMode();
        return;
    }  
}

Channel::Channel(uint32_t id): PHYSICAL_CHANNEL_ID(id) {
    assert(PHYSICAL_CHANNEL_ID >= 1 && PHYSICAL_CHANNEL_ID <= PHYSICAL_CHANNEL_COUNT);
    m_mainAddress = 1;
    m_subAddress = 1;
}

void ChannelGroup::UpdateWatchList() {
    MaIPCPacket packet;
    packet.type = IPCMessageType::UPDATE_ENCODER_WATCHLIST;

    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        auto channel_data = &packet.payload.EncoderRequest[i];

        channel_data->channel = m_channels[i].m_subAddress;
        channel_data->page = m_channels[i].m_mainAddress;
    }

    if(cb_RequestMaData) { cb_RequestMaData(packet); }
}

void ChannelGroup::ChangePage(int32_t pageOffset) {
    assert(pageOffset == -1 || pageOffset == 1);

    uint32_t original_page = m_page;
    if (pageOffset == -1 && m_page > 1) { m_page--;} 
    if (pageOffset == 1 && m_page < MAX_PAGE_COUNT) { m_page++; } 
    if (m_page == original_page) { return; }
    
    // When changing page, we reset the channel subaddress
    m_channelOffset = 0;
    uint32_t m_not_pinned_i = 1;

    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        auto &channel = m_channels[i];
        if (channel.IsPinned()) { continue; }
        channel.m_mainAddress = m_page;
        channel.m_subAddress = m_not_pinned_i++;
    }

    UpdateWatchList();

    if(cb_RequestMaData) {
        MaIPCPacket packet;
        packet.type = IPCMessageType::SET_PAGE;
        packet.payload.page = m_page;
        cb_RequestMaData(packet);
    }
}

void ChannelGroup::ScrollPage(int32_t scrollOffset) {
    assert(scrollOffset == -1 || scrollOffset == 1);

    uint32_t original_offset = m_channelOffset;
    if (scrollOffset == -1 && m_channelOffset > 1) { m_channelOffset--;} else { m_channelOffset++; }
    if (m_channelOffset == original_offset) { return; }

    uint32_t width = 0;
    for(int i = 0; i < 8; i++) {if (!m_channels[i].IsPinned()) { width++; }}
    uint32_t adjusted_base_subAddress = 1 + (width * m_channelOffset);

    if ((adjusted_base_subAddress + width) > 90) {
        // Revert and return
        m_channelOffset--;
        return;
    }

    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        auto &channel = m_channels[i];
        if (channel.IsPinned()) { continue; }
        channel.m_mainAddress = m_page;
        channel.m_subAddress = adjusted_base_subAddress + i;
    }

    UpdateWatchList();
}

void ChannelGroup::TogglePinConfigMode() {
    m_pinConfigMode = !m_pinConfigMode;
    // if (m_pinConfigMode) {printf("Entering pin config mode\n"); }
    // if (!m_pinConfigMode) {printf("Exiting pin config mode\n"); }
    if (!cb_SendButtonLightState) { return; } // No callback for sending light state data, can't do anything

    if (m_pinConfigMode) {
        cb_SendButtonLightState(static_cast<xt_buttons>(xt_alias_btn::PIN), xt_button_state_t::ON);
        for(int i = 0; i < sizeof(m_channels); i++) {
            auto select_btn = static_cast<xt_buttons>(FADER_0_SELECT + i);
            auto mute_btn = static_cast<xt_buttons>(FADER_0_MUTE + i);

            if (m_channels[i].IsPinned()) {
                cb_SendButtonLightState(mute_btn, xt_button_state_t::FLASHING);
            } else {
                cb_SendButtonLightState(select_btn, xt_button_state_t::FLASHING);
            }
        }
    }
    else {
        cb_SendButtonLightState(static_cast<xt_buttons>(xt_alias_btn::PIN), xt_button_state_t::OFF);
        for(int i = 0; i < sizeof(m_channels); i++) {
            auto select_btn = static_cast<xt_buttons>(FADER_0_SELECT + i);
            auto mute_btn = static_cast<xt_buttons>(FADER_0_MUTE + i);
            cb_SendButtonLightState(select_btn, xt_button_state_t::OFF);
            cb_SendButtonLightState(mute_btn, xt_button_state_t::OFF);
        }
    }
}

ChannelGroup::ChannelGroup() {
    m_channels = (Channel*)(malloc(sizeof(Channel) * PHYSICAL_CHANNEL_COUNT));
    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        auto channel = new (&m_channels[i]) Channel(i + 1);
        channel->m_subAddress = i + 1;
    }
    m_page = 1;
    m_channelOffset = 0;
}

void ChannelGroup::RegisterMAOutCB(std::function<void(MaIPCPacket&)> requestCb) {
    cb_RequestMaData = requestCb;
}

void ChannelGroup::RegisterButtonLightState(std::function<void(xt_buttons, xt_button_state_t)> lightCb) {
    cb_SendButtonLightState = lightCb;
}

void Channel::Pin(bool state) {
    m_pinned = state;
}

bool Channel::IsPinned() {
    return m_pinned;
}
