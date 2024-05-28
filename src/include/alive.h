#pragma once

class Alive {
private:
    bool m_Alive = false;
public:
    bool IsAlive();
    void SetLifeState(bool Alive);
};