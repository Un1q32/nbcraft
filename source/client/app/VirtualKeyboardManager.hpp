#pragma once

#include "AppPlatform.hpp"
#include "VirtualKeyboard.hpp"

// Defers requests for a keyboard until the Manager's next tick
class VirtualKeyboardManager
{
private:
    struct State
    {
        State()
        {
            playerId = 0;
            bVisible = false;
        }
        
        LocalPlayerID playerId;
        VirtualKeyboard keyboard;
        bool bVisible;
    };
    
public:
    VirtualKeyboardManager();
    
private:
    void _enqueue(LocalPlayerID playerId, const VirtualKeyboard* pKeyboard);
    void _applyState();
    
public:
    void tick();
    
	void showKeyboard(LocalPlayerID playerId, const VirtualKeyboard& keyboard);
	void hideKeyboard(LocalPlayerID playerId);
    
private:
    bool m_bHasQueuedState;
    
    State m_queuedState;
};