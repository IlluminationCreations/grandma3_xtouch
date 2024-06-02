#pragma once
#include <stdint.h>

struct Address {
    uint32_t mainAddress;
    uint32_t subAddress;

};
bool operator<(const Address& a, const Address& b);
