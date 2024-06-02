#pragma once
#include <guards.h>

constexpr unsigned short xt_port = 10111;
constexpr unsigned int PHYSICAL_CHANNEL_COUNT = 8;
// constexpr unsigned int MAX_PAGE_COUNT = 9999;
constexpr unsigned int MAX_PAGE_COUNT = 99; // TODO: Limiting to 99 as the assignment display only has 2 digits. Do we really need 9999 pages?
constexpr unsigned int MAX_CHANNEL_COUNT = 90;
enum class ControlType { UNKNOWN, SEGMENT, FADER, KNOB };
enum class SegmentID { UNKNOWN, PAGE };