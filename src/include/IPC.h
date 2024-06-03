
#pragma once
#define IPC_STRUCT struct __attribute__((__packed__))

namespace IPC {
    namespace PacketType {
        enum Type 
        {
            REQ_ENCODERS = 0x8000, 
            RESP_ENCODERS = 0x8001, 
            UPDATE_MA_ENCODER = 0x8002, 
            UPDATE_MA_MASTER = 0x8003,
            PRESS_MA_KEY = 0x8004,
            END = 0x8005,
        };
    }

    IPC_STRUCT IPCHeader {
        PacketType::Type type;
        uint32_t seq;
    };

    namespace PlaybackRefresh {
        // =============================================
        // ============== REQ_ENCODERS =================
        // =============================================
        IPC_STRUCT Request {
            IPC_STRUCT {
                unsigned int page;
                unsigned int channel; // eg x01, x02, x03
            } EncoderRequest[8];
        };

        // =============================================
        // ============== RESP_ENCODERS ================
        // =============================================
        IPC_STRUCT ChannelMetadata {
            float master; // Master fader
            bool channelActive[8]; // True if channel/playback has any active encoders or keys
        }; 

        // Represents the entire column of encoders and keys for a single playback
        IPC_STRUCT Data {
            uint16_t page;
            uint8_t channel; // eg x01, x02, x03
            IPC_STRUCT {
                bool isActive;
                char key_name[8];
                float value;
            } Encoders[3]; // 4xx, 3xx, 2xx encoders
            bool keysActive[4]; // 4xx, 3xx, 2xx, 1xx keys are being used
        };
    }

    namespace EncoderUpdate {
        IPC_STRUCT Data {
            uint16_t page;
            uint8_t channel; // eg x01, x02, x03
            uint16_t encoderType; // 400, 300, 200, 100
            float value;
        };

        IPC_STRUCT MasterData {
            float value;
        };
    }

}