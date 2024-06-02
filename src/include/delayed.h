#pragma once
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

using RegistrationId = uint32_t;

class DelayedExecuter {
private:
    using clock = std::chrono::high_resolution_clock;
    using time_point = std::chrono::time_point<clock>;

    struct Execution {
        RegistrationId id;
        bool active;
        float value;
        time_point lastTime;
        uint32_t delayDuration;
        std::function<void(float)> callback;
    };
    // Used to protect m_delayedExecutions
    std::mutex m_mutex_data;
    std::vector<Execution> m_delayedExecutions;

    std::thread m_thread;
    // Used to block thread when no executions are active 
    std::mutex m_mutex_note; 
    // Used to wake up thread when new execution is added based on m_mutex_note
    std::condition_variable m_condition;
    bool m_suspended = false;
    void _threadimpl();
    void _wait();
    // Wake up thread, if thread is suspended
    void _maybe_wake(); 

public:
    DelayedExecuter();
    RegistrationId Register(std::function<void(float)> callback, uint32_t delayDuration);
    void Update(RegistrationId id, float value);
};

extern DelayedExecuter *g_delayedThreadScheduler;