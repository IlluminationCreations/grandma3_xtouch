/* ----------------------------------------------------------------
                   x-touch-xctl library.
   This library allows you to communicate with a Behringer X-Touch 
   over Ethernet using the Xctl protocol. It gives you full control
   over the motorised faders, LEDs, 7-segment displays, wheels
   and scribble pads (including RGB backlight control)
   ---------------------------------------------------------------- */
   
/* Note: This library doesn't contain the routines for actually sending
   and receiving the UDP packets over Ethernet (it only generates and
   interprets the packet contents). For an example of how to implement
   this see the main.cpp file supplied alongside */

/*
MIT License

Copyright (c) 2020 Martin Whitaker

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include <time.h>
#include <functional>
#include <thread>
#include <chrono>

using PacketCallback = std::function<void(unsigned char*, uint64_t)>;
using EventCallback = std::function<void(unsigned char, int)>;

enum xt_colours_t { BLACK, RED, GREEN, YELLOW, BLUE, PINK, CYAN, WHITE };
enum xt_button_state_t { OFF, FLASHING, ON };
enum xt_buttons {
    FADER_0_MUTE = 16,
    FADER_0_REC = 0,
    FADER_0_SELECT = 24,
    FADER_0_SOLO = 8,
    FADER_0_DIAL_PRESS = 32,

    FADER_1_MUTE = 17,
    FADER_1_REC = 1,
    FADER_1_SELECT = 25,
    FADER_1_SOLO = 9,
    FADER_1_DIAL_PRESS = 33,

    FADER_2_MUTE = 18,
    FADER_2_REC = 2,
    FADER_2_SELECT = 26,
    FADER_2_SOLO = 10,
    FADER_2_DIAL_PRESS = 34,

    FADER_3_MUTE = 19,
    FADER_3_REC = 3,
    FADER_3_SELECT = 27,
    FADER_3_SOLO = 11,
    FADER_3_DIAL_PRESS = 35,

    FADER_4_MUTE = 20,
    FADER_4_REC = 4,
    FADER_4_SELECT = 28,
    FADER_4_SOLO = 12,
    FADER_4_DIAL_PRESS = 36,

    FADER_5_MUTE = 21,
    FADER_5_REC = 5,
    FADER_5_SELECT = 29,
    FADER_5_SOLO = 13,
    FADER_5_DIAL_PRESS = 37,

    FADER_6_MUTE = 22,
    FADER_6_REC = 6,
    FADER_6_SELECT = 30,
    FADER_6_SOLO = 14,
    FADER_6_DIAL_PRESS = 38,

    FADER_7_MUTE = 23,
    FADER_7_REC = 7,
    FADER_7_SELECT = 31,
    FADER_7_SOLO = 15,
    FADER_7_DIAL_PRESS = 39,

    ENCODER_TRACK = 40,
    ENCODER_PAN_SUR = 41,
    ENCODER_EQ = 42,
    ENCODER_SEND = 43,
    ENCODER_PLUGIN = 44,
    ENCODER_INST = 45,

    FADER_BANK_LEFT = 46,
    FADER_BANK_RIGHT = 47,

    CHANNEL_LEFT = 48,
    CHANNEL_RIGHT = 49,

    FUNCTION_F1 = 54,
    FUNCTION_F2 = 55,
    FUNCTION_F3 = 56,
    FUNCTION_F4 = 57,
    FUNCTION_F5 = 58,
    FUNCTION_F6 = 59,
    FUNCTION_F7 = 60,
    FUNCTION_F8 = 61,

    MODIFY_SHIFT = 70,
    MODIFY_OPTION = 71,
    MODIFY_CONTROL = 72,
    MODIFY_ALT = 73,

    AUTOMATION_READ = 74,
    AUTOMATION_WRITE = 75,
    AUTOMATION_TRIM = 76,
    AUTOMATION_TOUCH = 77,
    AUTOMATION_LATCH = 78,
    AUTOMATION_GROUP = 79,

    UTILITY_SAVE = 80,
    UTILITY_UNDO = 81,
    UTILITY_CANCEL = 82,
    UTILITY_ENTER = 83,

    TRANSPORT_MARKER = 84,
    TRANSPORT_NUDGE = 85,
    TRANSPORT_CYCLE = 86,
    TRANSPORT_DROP = 87,
    TRANSPORT_REPLACE = 88,
    TRANSPORT_CLICK = 89,
    TRANSPORT_SOLO = 90,

    PLAYBACK_REWIND = 91,
    PLAYBACK_FAST_FORWARD = 92,
    PLAYBACK_STOP = 93,
    PLAYBACK_PLAY = 94,
    PLAYBACK_RECORD = 95,

    CURSOR_UP = 96,
    CURSOR_DOWN = 97,
    CURSOR_LEFT = 98,
    CURSOR_RIGHT = 99,
    CURSOR_MIDDLE = 100,

    // Buttons that aren't in a group
    SCRUB = 101,
    MIDI_TRACKS = 62,
    INPUTS = 63,
    AUDIO_TRACKS = 64,
    AUDIO_INST = 65,
    AUX = 66,
    BUSES = 67,
    OUTPUTS = 68,
    USER = 69,
    FLIP = 50,
    GLOBAL_VIEW = 51,

    // Lights
    SMPTE = 113,
    BEATS = 114,
    SOLO = 115,
    END
};
enum xt_alias_btn {
    PIN = xt_buttons::FLIP,
    PAGE_DEC = xt_buttons::CHANNEL_LEFT,
    PAGE_INC = xt_buttons::CHANNEL_RIGHT,
    EXECUTER_SCROLL_RIGHT = xt_buttons::FADER_BANK_RIGHT,
    EXECUTER_SCROLL_LEFT = xt_buttons::FADER_BANK_LEFT
};

typedef struct {
    char TopText[8];
    char BotText[8];
    xt_colours_t Colour;
    int Inverted;
} xt_ScribblePad_t;

namespace ButtonUtils {
    enum class ButtonType : uint16_t {
        REC = 300,
        SOLO = 200,
        MUTE = 100,
        SELECT = 0,
    };
    struct ButtonInfo {
        ButtonType buttonType;
        int channel;
    };
    int RecButtonToChannel(xt_buttons button);
    int SoloButtonToChannel(xt_buttons button);
    int MuteButtonToChannel(xt_buttons button);
    int SelectButtonToChannel(xt_buttons button);
    ButtonInfo FaderButtonToButtonType(xt_buttons button);
    bool AddressChangingButton(xt_buttons button);
}

class XTouch {
    public:
        XTouch();
        ~XTouch();

        int HandlePacket(unsigned char *buffer, unsigned int len);
        void SetAssignment(int v);
        void SetHMSF(int h, int m, int s, int f);
        void SetFrames(int v);
        void SetTime(struct tm* t);
        void SetDialPan(int channel, int position);
        void SetDialLevel(int channel, int level);
        void SetFaderLevel(int channel, int level);
        void SetMeterLevel(int channel, int level);
        void SetSingleButton(unsigned char n, xt_button_state_t v);
        void SetScribble(int channel, xt_ScribblePad_t info);
        void RegisterPacketSender(PacketCallback handler);     
        void ClearButtonLights();

    private:
        friend class InterfaceManager;
        void RegisterFaderCallback(EventCallback Handler);
        void RegisterDialCallback(EventCallback Handler);
        void RegisterButtonCallback(EventCallback Handler); 

        void RegisterFaderTouch(EventCallback Handler);

        int HandleFaderTouch(unsigned char *buffer, unsigned int len);
        int HandleLevel(unsigned char *buffer, unsigned int len);
        int HandleRotation(unsigned char *buffer, unsigned int len);
        int HandleButton(unsigned char *buffer, unsigned int len);
        int HandleProbe(unsigned char *buffer, unsigned int len);
        int HandleUnknown(unsigned char *buffer, unsigned int len);
        void SendPacket(unsigned char *buffer, unsigned int len);
        void CheckIdle();
        void SendScribble(unsigned char n);
        void SendAllScribble();
        void SendSingleButton(unsigned char n);
        void SendAllButtons();
        void SendSingleDial(unsigned char n, int value);
        void SendSingleFader(unsigned char n);
        void SendAllFaders();
        void SendAllBoard();
        void SetSegments(unsigned char segment, unsigned char value);
        void SendSegments();
        void DisplayNumber(unsigned char start, int len, int v,int zeros=0);
        unsigned char SegmentBitmap(char v);
        void SendAllMeters();
        void SendAllMetersLoop();

        PacketCallback m_packetCallBack;
        EventCallback m_buttonCallBack;
        EventCallback m_dialCallBack;
        EventCallback m_faderStateCallBack;
        EventCallback m_faderCallBack;

        time_t mLastIdle;
        int mFullRefreshNeeded;
        xt_button_state_t mButtonLEDStates[127];
        unsigned char mMeterLevels[8];
        unsigned int mFaderLevels[9];
        unsigned char mSegmentCache[12];

        xt_ScribblePad_t mScribblePads[8];

        std::thread m_soundMeterRefresh;
};

extern XTouch *g_xtouch;