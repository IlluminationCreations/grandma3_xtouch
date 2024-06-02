#pragma once

template<typename T>
class Observer {
    T m_page;
    std::function<void(T)> m_updateCb;
public:
    Observer(T page, std::function<void(T)> updateCb) {
        m_updateCb = updateCb;
        Set(page);
    }
    T Get() {
        return m_page;
    }
    void Set(T page) {
        m_page = page;
        m_updateCb(m_page);
    }
};