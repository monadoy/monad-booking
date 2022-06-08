#include "epdgui_button.h"

EPDGUI_Button::EPDGUI_Button(int16_t x, int16_t y, int16_t w, int16_t h): 
EPDGUI_Base(x, y, w, h)
{
    this->_CanvasNormal = new M5EPD_Canvas(&M5.EPD);
    this->_CanvasPressed = new M5EPD_Canvas(&M5.EPD);
    this->_CanvasNormal->createCanvas(_w, _h);
    this->_CanvasPressed->createCanvas(_w, _h);
}

EPDGUI_Button::EPDGUI_Button(String label, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, uint16_t color_pressed, uint16_t txt_color, bool use_bold, uint32_t style): 
EPDGUI_Base(x, y, w, h)
{
    if(style & STYLE_INVISABLE)
    {
        _is_invisable = true;
        return;
    }
    _use_bold = use_bold;
    _thiscreatNormal = false;
    _thiscreatPressed = false;
    _size = 28;
    _color = color;
    _txt_color = txt_color;
    _color_pressed = color_pressed;

    this->_label = label;
    
    this->_CanvasNormal = new M5EPD_Canvas(&M5.EPD);
    this->_CanvasPressed = new M5EPD_Canvas(&M5.EPD);

    if(!_CanvasNormal->isRenderExist(_size)){
        _CanvasNormal->createRender(_size, 120);
        _thiscreatNormal = true;
    }

    if(!_CanvasPressed->isRenderExist(_size)){
        _CanvasPressed->createRender(_size, 120);
        _thiscreatPressed = true;
    }

    // log_d("[%s] %d, %d", label.c_str(), _x, _y);
    
    this->_CanvasNormal->createCanvas(_w, _h);
    this->_CanvasPressed->createCanvas(_w, _h);

    this->_CanvasNormal->fillCanvas(_color);
    this->_CanvasNormal->setTextSize(_size);
    this->_CanvasNormal->setTextColor(_txt_color);

    this->_CanvasPressed->fillCanvas(_color_pressed);
    this->_CanvasPressed->setTextSize(_size);
    this->_CanvasPressed->setTextColor(_txt_color);

    for (int i = 0; i < 26; i++)
    {
        _CanvasPressed->preRender('a' + i);
        _CanvasPressed->preRender('A' + i);
        _CanvasNormal->preRender('a' + i);
        _CanvasNormal->preRender('A' + i);
    }

    if(style & STYLE_SOLIDBORDER)
    {
        this->_CanvasNormal->drawRect(0, 0, _w, _h, 15);
    }
    
    if(style & STYLE_ALIGN_LEFT)
    {
        this->_CanvasNormal->setTextDatum(CL_DATUM);
        this->_CanvasPressed->setTextDatum(CL_DATUM);
        this->_CanvasNormal->drawString(_label,  5, _h / 2 + 3);
        this->_CanvasPressed->drawString(_label, 5, _h / 2 + 3);
    }
    else if(style & STYLE_ALIGN_RIGHT)
    {
        this->_CanvasNormal->setTextDatum(CR_DATUM);
        this->_CanvasPressed->setTextDatum(CR_DATUM);
        this->_CanvasNormal->drawString(_label,  _w - 5, _h / 2 + 3);
        this->_CanvasPressed->drawString(_label, _w - 5, _h / 2 + 3);
    }
    else if(style & STYLE_ALIGN_CENTER)
    {
        this->_CanvasNormal->setTextDatum(CC_DATUM);
        this->_CanvasPressed->setTextDatum(CC_DATUM);
        this->_CanvasNormal->drawString(_label,  _w / 2, _h / 2 + 3);
        this->_CanvasPressed->drawString(_label, _w / 2, _h / 2 + 3);
    }
}

EPDGUI_Button::~EPDGUI_Button()
{
    delete this->_CanvasNormal;
    delete this->_CanvasPressed;
}

M5EPD_Canvas* EPDGUI_Button::CanvasNormal()
{
    return this->_CanvasNormal;
}

M5EPD_Canvas* EPDGUI_Button::CanvasPressed()
{
    return this->_CanvasPressed;
}

void EPDGUI_Button::Draw(m5epd_update_mode_t mode)
{
    if(_ishide || _is_invisable)
    {
        return;
    }

    if(_state == EVENT_NONE || _state == EVENT_RELEASED)
    {
        this->_CanvasNormal->pushCanvas(_x, _y, mode);
    }
    else if(_state == EVENT_PRESSED)
    {
        this->_CanvasPressed->pushCanvas(_x, _y, mode);
    }
}

void EPDGUI_Button::Draw(M5EPD_Canvas* canvas)
{
    if(_ishide)
    {
        return;
    }

    if(_state == EVENT_NONE || _state == EVENT_RELEASED)
    {
        _CanvasNormal->pushToCanvas(_x, _y, canvas);
    }
    else if(_state == EVENT_PRESSED)
    {
        _CanvasPressed->pushToCanvas(_x, _y, canvas);
    }
}

void EPDGUI_Button::Bind(int16_t event, void (* func_cb)(epdgui_args_vector_t&))
{
    if(event == EVENT_PRESSED)
    {
        _pressed_cb = func_cb;
    }
    else if(event == EVENT_RELEASED)
    {
        _released_cb = func_cb;
    }
}

bool EPDGUI_Button::UpdateState(int16_t x, int16_t y)
{
    if(!_isenable || _ishide)
    {
        return false;
    }

    bool is_in_area = isInBox(x, y);

    if(is_in_area)
    {
        if(_state == EVENT_NONE)
        {
            _state = EVENT_PRESSED;
            // Serial.printf("%s Pressed ", _label.c_str());
            Draw();
            if(_pressed_cb != NULL)
            {
                _pressed_cb(_pressed_cb_args);
            }
            
        }
        return true;
    }
    else
    {
        if(_state == EVENT_PRESSED)
        {
            _state = EVENT_NONE;
            Draw();
            if(_released_cb != NULL)
            {
                _released_cb(_released_cb_args);
            }
        }
        return false;
    }
}

void EPDGUI_Button::setBMPButton(String label_l, String label_r, const uint8_t *bmp32x32)
{
    _CanvasNormal->fillCanvas(_color);
    _CanvasNormal->drawRect(0, 0, _w, _h, _color);
    _CanvasNormal->setTextSize(_size);
    _CanvasNormal->setTextColor(_txt_color);
    if(label_l.length())
    {
        _CanvasNormal->setTextDatum(CL_DATUM);
        _CanvasNormal->drawString(label_l, 47 + 8, (_h >> 1) + 5);
    }
    if(label_r.length())
    {
        _CanvasNormal->setTextDatum(CR_DATUM);
        _CanvasNormal->drawString(label_r, _w - 15, (_h >> 1) + 5);
    }
    _CanvasNormal->pushImage(15, (_h >> 1) - 16, 32, 32, bmp32x32);
    *(_CanvasPressed) = *(_CanvasNormal);
    _CanvasPressed->ReverseColor();
}

void EPDGUI_Button::setLabel(String label)
{
    _label = label;
    this->_CanvasNormal->fillCanvas(_color);
    this->_CanvasNormal->drawRect(0, 0, _w, _h, _color);
    this->_CanvasNormal->setTextSize(_size);
    this->_CanvasNormal->setTextDatum(CC_DATUM);
    this->_CanvasNormal->setTextColor(_txt_color);
    this->_CanvasNormal->drawString(_label,  _w / 2, _h / 2 + 3);

    this->_CanvasPressed->fillCanvas(_color);
    this->_CanvasPressed->setTextSize(_size);
    this->_CanvasPressed->setTextDatum(CC_DATUM);
    this->_CanvasPressed->setTextColor(_txt_color);
    this->_CanvasPressed->drawString(_label, _w / 2, _h / 2 + 3);
}

void EPDGUI_Button::AddArgs(int16_t event, uint16_t n, void* arg)
{
    if(event == EVENT_PRESSED)
    {
        if(_pressed_cb_args.size() > n)
        {
            _pressed_cb_args[n] = arg;
        }
        else
        {
            _pressed_cb_args.push_back(arg);
        }
    }
    else if(event == EVENT_RELEASED)
    {
        if(_released_cb_args.size() > n)
        {
            _released_cb_args[n] = arg;
        }
        else
        {
            _released_cb_args.push_back(arg);
        }
    }
}

void EPDGUI_Button::setInvisable(bool isinvisable)
{
    _is_invisable = isinvisable;
}
