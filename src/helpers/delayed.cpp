#include <delayed.h>
DelayedExecuter::DelayedExecuter() {
    m_thread = std::thread(&DelayedExecuter::_threadimpl, this);
    m_thread.detach();

}

void DelayedExecuter::_wait() {
    std::unique_lock<std::mutex> lock(m_mutex_note);
    m_suspended = true;
    m_condition.wait(lock, [this] { return !m_suspended; });
}

void DelayedExecuter::_maybe_wake() {
    std::unique_lock<std::mutex> lock(m_mutex_note);
    if (!m_suspended) { return; } // Thread is not suspended, no need to wake it up

    m_suspended = false;
    m_condition.notify_all();    
}

void DelayedExecuter::_threadimpl() {
    using namespace std::chrono;

    while (true) {
        bool hasActive = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex_data);
            auto now = clock::now();
            for (auto &execution : m_delayedExecutions) {
                if (!execution.active) { continue; } else { hasActive = true; }

                auto duration = duration_cast<milliseconds>(now - execution.lastTime).count();
                if (duration >= execution.delayDuration) {
                    execution.active = false;
                    execution.callback(execution.value);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        if(!hasActive) { _wait(); }
    }
}

RegistrationId DelayedExecuter::Register(std::function<void(float)> callback, uint32_t delayDuration) {
    std::lock_guard<std::mutex> lock(m_mutex_data);
    Execution execution;
    execution.id = m_delayedExecutions.size();
    execution.active = false;
    execution.delayDuration = delayDuration;
    execution.lastTime = clock::now();
    execution.callback = callback;
    m_delayedExecutions.push_back(execution);

    return execution.id;
}

// Update the value of a delayed execution, only if the value has changed
void DelayedExecuter::Update(RegistrationId id, float value) {
    std::lock_guard<std::mutex> lock(m_mutex_data);
    auto &execution = m_delayedExecutions[id];

    if (execution.value == value) { return; }

    execution.value = value;
    execution.lastTime = clock::now(); // Reset time when turning active 

    if (!execution.active) { 
        execution.active = true;
    }

    _maybe_wake();
}

void DelayedExecuter::ForcedUpdate(RegistrationId id, float value) {
    std::lock_guard<std::mutex> lock(m_mutex_data);
    auto &execution = m_delayedExecutions[id];

    if (execution.value == value) { return; }

    execution.value = value;
    execution.active = false;
    execution.callback(execution.value);
}