#pragma once
#include <functional>

template<typename T>
class Observer {
    T stored_value;
    std::function<void(T)> m_updateCb;
public:
    Observer(T value, std::function<void(T)> updateCb) {
        m_updateCb = updateCb;
        Set(value);
    }
    T Get() {
        return stored_value;
    }
    void Set(T value) {
        stored_value = value;
        m_updateCb(value);
    }
};