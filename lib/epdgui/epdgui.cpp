#include <stack>
#include <map>
#include <list>
#include "epdgui.h"


std::list<EPDGUI_Base*> epdgui_object_list;
uint32_t obj_id = 1;
bool _is_auto_update = true;
uint16_t _last_pos_x = 0xFFFF, _last_pos_y = 0xFFFF;

void EPDGUI_AddObject(EPDGUI_Base* object)
{
    obj_id++;
    epdgui_object_list.push_back(object);
}

void EPDGUI_Draw(m5epd_update_mode_t mode)
{
    for(std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin(); p != epdgui_object_list.end(); p++)
    {
        (*p)->Draw(mode);
    }
}

void EPDGUI_Process(void)
{
    for(std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin(); p != epdgui_object_list.end(); p++)
    {
        (*p)->UpdateState(-1, -1);
    }
}

void EPDGUI_Process(int16_t x, int16_t y)
{
    for(std::list<EPDGUI_Base*>::iterator p = epdgui_object_list.begin(); p != epdgui_object_list.end(); p++)
    {
        // log_d("%d, %d -> %d, %d, %d, %d", x, y, (*p)->getX(), (*p)->getY(), (*p)->getRX(), (*p)->getBY());
        (*p)->UpdateState(x, y);
    }
}

void EPDGUI_Clear(void)
{
    epdgui_object_list.clear();
}

void EPDGUI_SetAutoUpdate(bool isAuto)
{
    _is_auto_update = isAuto;
}
