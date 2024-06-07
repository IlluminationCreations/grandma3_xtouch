#include <interface.h>
#include <standard.h>
#include <stdio.h>

class BaseLayer : public InterfaceLayer {
public:
    void Resume() override;
    void Pause() override;
    void Start() override;
    void Removed() override;
    bool HandleInput(PhysicalEvent event) override;
};
void BaseLayer::Resume() {}
void BaseLayer::Pause() {}
void BaseLayer::Start() {}
void BaseLayer::Removed() {
    assert(false && "Base layer should never be removed");
}
bool BaseLayer::HandleInput(PhysicalEvent event) {
    switch(event.type) {
        case PhysicalEventType::FADER: {
            printf("[BaseLayer] Fader %u hit, state = %u\n", event.data.faderDial.Column, event.data.faderDial.value);
            break;
        }
        case PhysicalEventType::DIAL: {
            printf("[BaseLayer] Dial %u hit, state = %d\n", event.data.faderDial.Column, event.data.faderDial.value);
            break;
        }
        case PhysicalEventType::FADER_BUTTON: {
            printf("[BaseLayer] Fader button %hu on channel %u hit, state = %u\n", event.data.faderButton.info.buttonType, event.data.faderButton.info.channel, event.data.faderButton.down);
            break;
        }
        case PhysicalEventType::DIAL_PRESS: {
            printf("[BaseLayer] Dial press %u hit, state = %u\n", event.data.faderDial.Column, event.data.faderDial.value);
            break;
        }
        case PhysicalEventType::BUTTON: {
            printf("[BaseLayer] Button %u hit, state = %u\n", event.data.button.Id, event.data.button.down);
            break;
        }
        case PhysicalEventType::MASTER: {
            printf("[BaseLayer] Master hit, state = %u\n", event.data.master.value);
            break;
        }
        case PhysicalEventType::JOG: {
            printf("[BaseLayer] Jog hit, state = %d\n", event.data.jog.value);
            break;
        }
    }
    return true;
}

InterfaceManager::InterfaceManager(XTouch *xtouch) {
    xtouch->RegisterButtonCallback([&](unsigned char button, int attr){ ReceiveButton(button, attr); });
    xtouch->RegisterDialCallback([&](unsigned char button, int attr){ ReceiveDial(button, attr); });
    xtouch->RegisterFaderCallback([&](unsigned char button, int attr){ ReceiveFader(button, attr); });

    m_layers.push_back(new BaseLayer()); // Base layer
}

void InterfaceManager::ReceiveButton(unsigned char button, int attr) 
{
    // We need to handle 3 categories of buttons:
    // * Dial buttons
    // * Fader bank buttons
    // * System buttons

    // Dial buttons (dial being pressed in)
    bool isDialPress = button >= FADER_0_DIAL_PRESS && button <= FADER_7_DIAL_PRESS;
    // This means REC, SOLO, MUTE, SELECT
    bool isFaderButton = button >= FADER_0_REC && button <= FADER_7_SELECT;


    PhysicalEvent event;
    if (isDialPress) 
    {
        event.type = PhysicalEventType::DIAL_PRESS;
        event.data.faderDial.value = attr;
        event.data.faderDial.Column = button - FADER_0_DIAL_PRESS;
        assert(event.data.faderDial.Column >= 0 && event.data.faderDial.Column <= 7);
    }
    else if (isFaderButton) 
    {
        event.type = PhysicalEventType::FADER_BUTTON;
        event.data.faderButton.info = ButtonUtils::FaderButtonToButtonType(static_cast<xt_buttons>(button));
        event.data.faderButton.down = attr;
        assert(attr == 0 || attr == 1);
    }
    else // System buttons
    {
        event.type = PhysicalEventType::BUTTON;
        event.data.button.Id = button;
        event.data.button.down = attr;
        assert(attr == 0 || attr == 1);
    }
    DispatchEvent(event);
}

void InterfaceManager::ReceiveDial(unsigned char button, int attr) 
{   
    PhysicalEvent event;
    if (button >= 16 && button <= 23)
    {
        event.type = PhysicalEventType::DIAL;
        event.data.faderDial.Column = button - 16;
        event.data.faderDial.value = attr;
        assert(event.data.faderDial.Column >= 0 && event.data.faderDial.Column <= 7);
    } 
    else if (button == 60)
    {
        event.type = PhysicalEventType::JOG;
        event.data.jog.value = attr;
    }
    else {
        assert(false && "Invalid dial button");
    }
    DispatchEvent(event);
}

void InterfaceManager::ReceiveFader(unsigned char button, int attr) 
{
    assert(button >= 0 && button <= PHYSICAL_CHANNEL_COUNT);

    PhysicalEvent event;
    if (button == 8)
    {
        event.type = PhysicalEventType::MASTER;
        event.data.master.value = attr;
    } 
    else 
    {
        event.type = PhysicalEventType::FADER;
        event.data.faderDial.Column = button;
        event.data.faderDial.value = attr;
    }
    DispatchEvent(event);
}

void InterfaceManager::DispatchEvent(PhysicalEvent event) {
    for(auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        if ((*it)->HandleInput(event)) {
            return;
        }
    }
    assert(false && "No layer handled the event");
}

void InterfaceManager::PushLayer(InterfaceLayer *layer) {
    auto top = m_layers.back();
    top->Pause();
    m_layers.push_back(layer);
    layer->Start();
}

void InterfaceManager::PopLayer() {
    auto layer = m_layers.back();
    m_layers.pop_back();
    
    layer->Removed();
    assert(m_layers.size() > 0); // We should always have at the base layer

    m_layers.back()->Resume();
}
