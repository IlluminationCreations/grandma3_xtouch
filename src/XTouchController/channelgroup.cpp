#include <ChannelGroup.h>
#include <set>
#include <string.h>
#include <delayed.h>

void ChannelGroup::HandleAddressChange(xt_alias_btn btn) {
    switch (btn) {
        case xt_alias_btn::EXECUTER_SCROLL_LEFT: 
        case xt_alias_btn::EXECUTER_SCROLL_RIGHT: 
        {
            uint32_t offset = btn == xt_alias_btn::EXECUTER_SCROLL_LEFT ? -1 : 1;
            ScrollPage(offset);
            return;
        }
        case xt_alias_btn::PAGE_DEC:
        case xt_alias_btn::PAGE_INC: 
        {
            uint32_t offset = btn == xt_alias_btn::PAGE_DEC ? -1 : 1; 
            ChangePage(offset);
            return;
        }
        default: { assert(false); }
    }
}

std::vector<Address> ChannelGroup::CurrentChannelAddress() {
    std::vector<Address> addresses;
    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        addresses.push_back(m_channels[i].m_address->Get());
    }
    assert(addresses.size() == PHYSICAL_CHANNEL_COUNT);
    return std::move(addresses);
}

void ChannelGroup::DisablePhysicalChannel(uint32_t i) {
    assert(i >= 0 && i < 8);
    m_channels[i].Disable();
}

void ChannelGroup::UpdateEncoderFromMA(IPC::PlaybackRefresh::Data encoder, uint32_t physical_channel_id) {
    assert(physical_channel_id >= 0 && physical_channel_id < 8);
    auto &channel = m_channels[physical_channel_id];
    channel.UpdateEncoderFromMA(encoder);
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
            GenerateChannelWindows();
            return;
        }
        if (selectButtonHit) {
            printf("Pinning channel %u\n", selectBtnToChannel);
            m_channels[selectBtnToChannel].Pin(true);
            GenerateChannelWindows();
            return;
        }
        return;
    } 
    if (button == xt_alias_btn::PIN) {
        TogglePinConfigMode();
        return;
    }  
}

void ChannelGroup::UpdateWatchList() {
    auto buffer_size = sizeof(IPC::IPCHeader) + sizeof(IPC::PlaybackRefresh::Request);
    char *buffer = (char*)malloc(buffer_size);
    IPC::IPCHeader *header = (IPC::IPCHeader*)buffer;
    IPC::PlaybackRefresh::Request *packet = (IPC::PlaybackRefresh::Request*)(buffer + sizeof(IPC::IPCHeader));

    header->type = IPC::PacketType::REQ_ENCODERS;
    header->seq = 0; // TODO: Implement sequence number

    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        auto address = m_channels[i].m_address->Get();

        packet->EncoderRequest[i].channel = address.subAddress;
        packet->EncoderRequest[i].page = address.mainAddress;
    }

    if(cb_RequestMaData) { cb_RequestMaData(buffer, buffer_size); }
    free(buffer);
}

void ChannelGroup::ChangePage(int32_t pageOffset) {
    assert(pageOffset == -1 || pageOffset == 1);
    m_sequence = m_sequence + 1;

    uint32_t cur_page = m_page->Get();
    if (pageOffset == -1 && cur_page > 1) { m_page->Set(cur_page - 1);;} 
    if (pageOffset == 1 && cur_page < MAX_PAGE_COUNT) { m_page->Set(cur_page + 1); } 
    if (m_page->Get() == cur_page) { return; }
    GenerateChannelWindows();

    // When changing page, we reset the channel subaddress
    m_channelOffset = 0;

    auto mainAddress = m_page->Get();
    auto &window = m_channelWindows[m_channelOffset];
    auto it = window.begin();
    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        auto &channel = m_channels[i]; if (channel.IsPinned()) { continue; }
        assert(it != window.end());

        Address address;
        address.mainAddress = mainAddress;
        address.subAddress = *it;
        m_channels[i].m_address->Set(address);

        it++;
    }

    UpdateWatchList();
}

void ChannelGroup::ScrollPage(int32_t scrollOffset) {
    assert(scrollOffset == -1 || scrollOffset == 1);
    assert(m_channelWindows.size() > 0 && "Channel windows not generated");
    m_sequence = m_sequence + 1;

    uint32_t original_offset = m_channelOffset;
    if (scrollOffset == -1 && m_channelOffset > 0) { m_channelOffset--;}
    if (scrollOffset == 1 && m_channelOffset < m_channelOffsetEnd) { m_channelOffset++; }
    if (m_channelOffset == original_offset) { return; }

    auto mainAddress = m_page->Get();
    auto &window = m_channelWindows[m_channelOffset];
    auto &&it = window.begin();
    bool final_window = m_channelOffset == m_channelOffsetEnd;

    int i = 0; // We need to keep track of the number of channels we've updated, since we might not update all of them if we reach the final window
    // The final window might not have enough channels to fill all the physical channels.
    for(; i < PHYSICAL_CHANNEL_COUNT; i++) {
        auto &channel = m_channels[i]; if (channel.IsPinned()) { continue; }
        if (final_window && it == window.end()) { break; } // We've reached the final window, we need special handling
        assert(it != window.end()); // When we're not in the final window, we should never reach the end of the list

        Address address;
        address.mainAddress = mainAddress;
        address.subAddress = *it;
        m_channels[i].m_address->Set(address);

        it++;
    }
    if (final_window && i < PHYSICAL_CHANNEL_COUNT) {
        for(; i < PHYSICAL_CHANNEL_COUNT; i++) {
            Address address;
            address.mainAddress = mainAddress;
            address.subAddress = UINT32_MAX; // This is a special value that indicates that the channel is not valid
            m_channels[i].m_address->Set(address);
        }
    }
    assert(it == window.end());

    UpdateWatchList();
}

void ChannelGroup::RegisterMaSend(MaUDPServer *server) {
    m_maServer = server;
    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        m_channels[i].RegisterMaSend(server);
    }
}

void ChannelGroup::GenerateChannelWindows() {
    const auto pinned_addresses = [&]() -> std::set<Address> {
        auto pinned = std::set<Address>();
        uint32_t inserted = 0;

        for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
            auto &channel = m_channels[i]; 
            if (!channel.IsPinned()) { continue; }

            Address address = channel.m_address->Get();
            pinned.insert(address);
            inserted++;
        }

        assert(pinned.size() == inserted && "Pinned channels not unique");
        return std::move(pinned);
    }();

    const uint32_t window_width = PHYSICAL_CHANNEL_COUNT - pinned_addresses.size();

    std::vector<std::vector<uint32_t>> windows;
    std::vector<uint32_t> cur_window;
    auto cur_page = m_page->Get();
    for(unsigned int i = 1; i <= 90; i++) {
        if (cur_window.size() == window_width) {
            windows.push_back(cur_window);
            cur_window.clear();
        }

        Address address = {cur_page, i};
        if (pinned_addresses.find(address) != pinned_addresses.end()) { 
            continue; 
        }
        cur_window.push_back(i);
    }
    if (cur_window.size() > 0) { windows.push_back(cur_window); }
    m_channelWindows = std::move(windows);
    m_channelOffsetEnd = m_channelWindows.size() - 1;
}

void ChannelGroup::TogglePinConfigMode() {
    m_pinConfigMode = !m_pinConfigMode;
    if (m_pinConfigMode) {printf("Entering pin config mode\n"); }
    if (!m_pinConfigMode) {printf("Exiting pin config mode\n"); }
    assert(g_xtouch != nullptr && "XTouch instance not created");

    if (m_pinConfigMode) {
        g_xtouch->SetSingleButton(static_cast<xt_buttons>(xt_alias_btn::PIN), xt_button_state_t::ON);
        for(int i = 0; i < sizeof(m_channels); i++) {
            auto select_btn = static_cast<xt_buttons>(FADER_0_SELECT + i);
            auto mute_btn = static_cast<xt_buttons>(FADER_0_MUTE + i);

            if (m_channels[i].IsPinned()) {
                g_xtouch->SetSingleButton(mute_btn, xt_button_state_t::FLASHING);
            } else {
                g_xtouch->SetSingleButton(select_btn, xt_button_state_t::FLASHING);
            }
        }
    }
    else {
        g_xtouch->SetSingleButton(static_cast<xt_buttons>(xt_alias_btn::PIN), xt_button_state_t::OFF);
        for(int i = 0; i < sizeof(m_channels); i++) {
            auto select_btn = static_cast<xt_buttons>(FADER_0_SELECT + i);
            auto mute_btn = static_cast<xt_buttons>(FADER_0_MUTE + i);
            g_xtouch->SetSingleButton(select_btn, xt_button_state_t::OFF);
            g_xtouch->SetSingleButton(mute_btn, xt_button_state_t::OFF);
        }
    }
}

ChannelGroup::ChannelGroup() {
    m_channels = (Channel*)(malloc(sizeof(Channel) * PHYSICAL_CHANNEL_COUNT));
    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        auto channel = new (&m_channels[i]) Channel(i + 1);
    }
    m_masterFaderEncoder = new Encoder(EncoderId::Master, 0);

    m_page = new Observer<uint32_t>(1, [](uint32_t page) { 
        g_xtouch->SetAssignment(page);
    });
    
    GenerateChannelWindows();
    m_playbackRefresh = std::thread(&ChannelGroup::RefreshPlaybacks, this);
    m_playbackRefresh.detach();
}

void ChannelGroup::RegisterMAOutCB(std::function<void(char*, uint32_t)> requestCb) {
    cb_RequestMaData = requestCb;
}

void ChannelGroup::HandleButtonPress(char button, bool down) {
    xt_alias_btn btnAlias = static_cast<xt_alias_btn>(button); 
     if (ButtonUtils::AddressChangingButton(static_cast<xt_buttons>(button))) {  HandleAddressChange(btnAlias); return; }
     if (button >= FADER_0_DIAL_PRESS && button <= FADER_7_DIAL_PRESS) 
     { 
        auto i = button - FADER_0_DIAL_PRESS;
        m_channels[i].Toggle();
     }
}


void ChannelGroup::RefreshPlaybacks() {
    while (true) {
        RefreshPlaybacksImpl();

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

bool ChannelGroup::RefreshPlaybacksImpl() {
    using namespace std::chrono;

    if (!m_maServer) {return true;}

    IPC::IPCHeader header;
    header.type = IPC::PacketType::REQ_ENCODERS;
    header.seq = m_sequence;
    IPC::PlaybackRefresh::Request request;
    auto channels = CurrentChannelAddress();
    for(int i = 0; i < PHYSICAL_CHANNEL_COUNT; i++) {
        request.EncoderRequest[i].channel = channels[i].subAddress;
        request.EncoderRequest[i].page = channels[i].mainAddress;
    }

    auto packet_size = sizeof(IPC::IPCHeader) + sizeof(IPC::PlaybackRefresh::Request);
    char *buffer = (char*)malloc(4096);
    memcpy(buffer, &header, sizeof(IPC::IPCHeader));
    memcpy(buffer + sizeof(IPC::IPCHeader), &request, sizeof(IPC::PlaybackRefresh::Request));
    m_maServer->Send(buffer, packet_size);

    if (m_maServer->Read(buffer, 4096) < 0) {
        // assert(false && "Failed to read from MA server");
        printf("Failed to read from MA server\n");
        return false;
    }

    // ----------------- Past this line we reuse the buffer -----------------
    uint32_t offset = sizeof(IPC::IPCHeader);
    IPC::IPCHeader *resp_header = (IPC::IPCHeader*)(buffer);
    IPC::PlaybackRefresh::ChannelMetadata *resp_metadata = (IPC::PlaybackRefresh::ChannelMetadata*)(buffer + offset);
    offset += sizeof(IPC::PlaybackRefresh::ChannelMetadata);
    IPC::PlaybackRefresh::Data *data = (IPC::PlaybackRefresh::Data*)(buffer + offset);

    if (resp_header->type != IPC::PacketType::RESP_ENCODERS_META) {
        return false;
    }

    m_masterFaderEncoder->SetValue(resp_metadata->master, false);

    if (resp_header->seq != m_sequence) {
        // printf("Sequence number mismatch - dropping\n");
        return false;
    }

    uint32_t data_iter = 0;
    for(int i = 0; i < 8; i++) {
        if(!resp_metadata->channelActive[i]) {
            DisablePhysicalChannel(i);
            continue;
        }

        UpdateEncoderFromMA(data[data_iter++], i);    
    }
    free(buffer);
    return true;
}

void ChannelGroup::HandleUpdate(UpdateType type, char button, int value) {
    
    switch (type) {
        case UpdateType::FADER: 
        { 
            UpdateEncoderFromXT(button, value, true);
            break; 
        }
        case UpdateType::DIAL: 
        { 
            UpdateEncoderFromXT(button, value, false); 
            break; 
        }
        case UpdateType::BUTTON: {  break; }
        case UpdateType::MASTER: 
        {  
            UpdateMasterEncoder(value); 
            break;
        }
        default: { assert(false); }
    }
    m_sequence = m_sequence + 1;
}

void ChannelGroup::UpdateEncoderFromXT(uint32_t physical_channel_id, int value, bool isFader) {
    m_channels[physical_channel_id].UpdateEncoderFromXT(value, isFader);

}

void ChannelGroup::UpdateMasterEncoder(int value) {
    if (!m_maServer) {return;}
    auto normalized_value = (value / 16380.0f) * 100.0f; // 0.0f - 100.0f
    m_masterFaderEncoder->SetValue(normalized_value, true);

    IPC::IPCHeader header;
    header.type = IPC::PacketType::UPDATE_MA_MASTER;
    header.seq = 0; // TODO: Implement sequence number
    IPC::EncoderUpdate::MasterData packet;
    packet.value = normalized_value;

    auto packet_size = sizeof(IPC::IPCHeader) + sizeof(IPC::EncoderUpdate::Data);
    char *buffer = (char*)malloc(packet_size);
    memcpy(buffer, &header, sizeof(IPC::IPCHeader));
    memcpy(buffer + sizeof(IPC::IPCHeader), &packet, sizeof(IPC::EncoderUpdate::Data));
    m_maServer->Send(buffer, packet_size);
    free(buffer);
}
