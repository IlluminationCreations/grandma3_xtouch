#pragma once

class Alive {
private:
    bool m_Alive = true;
public:
    bool IsAlive();
    void SetDead();
};