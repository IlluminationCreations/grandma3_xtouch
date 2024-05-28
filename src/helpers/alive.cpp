#include <alive.h>

// Class that helps us identify when an object has died
bool Alive::IsAlive() {
    return m_Alive;
}

void Alive::SetDead() {
    m_Alive = false;
}