#pragma once
#include <list>
#include <x-touch.h>

namespace StructImpl {
    struct FaderDial 
    {
        uint16_t Column;
        int value;
    };
    
    struct FaderButton 
    {
        ButtonUtils::ButtonInfo info;
        int down;
    };

    struct Button 
    {
        uint16_t Id;
        int down;
    };

    struct Master 
    {
        int value;
    };

    struct Jog 
    {
        int value;
    };

    union InterfaceData 
    {
        // Fader, dial, dial press
        FaderDial faderDial;
        // REC, SOLO, MUTE, SELECT
        FaderButton faderButton;
        Button button;
        Master master;
        Jog jog;
    };

}

enum class PhysicalEventType {
    FADER,
    DIAL,
    FADER_BUTTON,
    DIAL_PRESS,

    BUTTON,

    MASTER,

    JOG
};

enum class FaderButtonType {
    REC, SOLO, MUTE, SELECT
};

struct PhysicalEvent 
{
    PhysicalEventType type;
    StructImpl::InterfaceData data;
};

struct InterfaceLayer {
    virtual void Resume() = 0;
    virtual void Pause() = 0;
    virtual void Start() = 0;
    virtual void Removed() = 0;
    virtual bool HandleInput(PhysicalEvent event) = 0;
};

class InterfaceManager {
public:
    void PushLayer(InterfaceLayer *layer);
    void PopLayer();
    InterfaceManager(XTouch *xtouch);
private:
    void ReceiveDial(unsigned char, int);
    void ReceiveFader(unsigned char, int);
    void ReceiveButton(unsigned char, int);
    void DispatchEvent(PhysicalEvent event);
    std::list<InterfaceLayer*> m_layers;
};

extern InterfaceManager *g_interfaceManager;