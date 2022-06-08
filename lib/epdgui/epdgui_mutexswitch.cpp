#include "epdgui_mutexswitch.h"

EPDGUI_MutexSwitch::EPDGUI_MutexSwitch()
{

}

void EPDGUI_MutexSwitch::Draw(m5epd_update_mode_t mode)
{
    if(_ishide)
    {
        return;
    }
    for(std::list<EPDGUI_Switch*>::iterator p = _object_list.begin(); p != _object_list.end(); p++)
    {
        (*p)->Draw(mode);
    }
}

void EPDGUI_MutexSwitch::Draw(M5EPD_Canvas* canvas)
{
    if(_ishide)
    {
        return;
    }
    for(std::list<EPDGUI_Switch*>::iterator p = _object_list.begin(); p != _object_list.end(); p++)
    {
        (*p)->Draw(canvas);
    }
}

void EPDGUI_MutexSwitch::Bind(int16_t event, void (* func_cb)(epdgui_args_vector_t&))
{

}

bool EPDGUI_MutexSwitch::UpdateState(int16_t x, int16_t y)
{
    return false; // this is deprecated but used nowhere
    if(!_isenable)
    {
        return false;
    }

    std::list<EPDGUI_Switch*>::iterator pressed_sw = _object_list.end();
    for(std::list<EPDGUI_Switch*>::iterator p = _object_list.begin(); p != _object_list.end(); p++)
    {
        if(_last_pressed == p)
        {
            (*p)->UpdateState(-1, -1);
            continue;
        }

        if((*p)->isInBox(x, y))
        {
            _last_pressed = p;
            pressed_sw = p;
        }
        
        (*p)->UpdateState(x, y);
    }

    if(!_is_exclusive)
    {
        return false;
    }

    if(pressed_sw == _object_list.end())
    {
        return false;
    }

    for(std::list<EPDGUI_Switch*>::iterator p = _object_list.begin(); p != _object_list.end(); p++)
    {
        if(pressed_sw == p)
        {
            continue;
        }

        if((*p)->getState() != 0)
        {   
            (*p)->setState(0);
            (*p)->Draw(UPDATE_MODE_GL16);
        }
    }
}

void EPDGUI_MutexSwitch::SetDefault(uint16_t idx)
{
    if(idx < _object_list.size())
    {
        _default_idx = idx;
    }
    
    int i = 0;
    for(std::list<EPDGUI_Switch*>::iterator p = _object_list.begin(); p != _object_list.end(); p++)
    {
        if(i == _default_idx)
        {
            _last_pressed = p;
            (*p)->setState(1);
            // (*p)->Draw(UPDATE_MODE_GL16);
        }
        else
        {
            (*p)->setState(0);
            // (*p)->Draw(UPDATE_MODE_GL16);
        }
        i++;
    }
}
