#ifndef ANIMMANAGER_H
#define ANIMMANAGER_H

#include <PNGdec.h>

namespace anim {
    
    const uint16_t NUM_OF_FRAMES = 14;

    class Animation {
        public:
            Animation();
            void showNextFrame(bool isReverse);
            void resetAnimation();
            void showLogo();
        private:
            void _drawFrame(bool isReverse);
            void _reverseDirection();
            int _currentFrame;
            int _direction;
    };
}

#endif