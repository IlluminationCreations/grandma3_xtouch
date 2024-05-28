#include <alive.h>

// Class that helps us identify when an object has died
bool Alive::IsAlive() {
    return m_Alive;
}

void Alive::SetLifeState(bool alive) {
    m_Alive = alive;
}