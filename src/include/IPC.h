
#pragma once
#define IPC_STRUCT struct __attribute__((__packed__))

namespace IPC {
    namespace PacketType {
        enum Type 
        {
            REQ_ENCODERS = 0x8000, 
            RESP_ENCODERS_META = 0x8001,
            // UNUSED = 0x8002,
            UPDATE_MA_ENCODER = 0x8003, 
            UPDATE_MA_MASTER = 0x8004,
            PRESS_MA_PLAYBACK_KEY = 0x8005,
            PRESS_MA_SYSTEM_KEY = 0x8006,
            END = 0x8007,
        };
    }

    IPC_STRUCT IPCHeader {
        PacketType::Type type;
        uint32_t seq;
    };

    namespace PlaybackRefresh {
        enum class EncoderType : uint16_t {
            x100 = 0x100,
            x200 = 0x200,
            x300 = 0x300,
            x400 = 0x400,
            None = 0x500
        };

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
                EncoderType type;
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

    namespace ButtonEvent {
        // Events for buttons attached to playbacks 
        IPC_STRUCT ExecutorButton {
            uint16_t page;
            uint8_t channel; // eg x01, x02, x03
            uint16_t type; // [0] 4xx, [1] 3xx, [2] 2xx, [3] 1xx
            bool down;
        };
        // Other buttons, like CLEAR
        enum class KeyType : uint32_t {
            CLEAR = 0x10101010,
            STORE = CLEAR + 1,
            UPDATE = STORE + 1,
            ASSIGN = UPDATE + 1,
            MOVE = ASSIGN + 1,
            OOPS = MOVE + 1,
            EDIT = OOPS + 1,
            DELETE = EDIT + 1,
            ESC = DELETE + 1,
        };

        IPC_STRUCT SystemKeyDown {
            KeyType key;
            bool down;
        };
    }
}