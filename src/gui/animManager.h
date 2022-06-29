#ifndef ANIMMANAGER_H
#define ANIMMANAGER_H

#include <PNGdec.h>

namespace anim {
    
    const uint16_t NUM_OF_FRAMES = 14;

    class Animation {
        public:
            Animation();
            void showNextFrame();
            void resetAnimation();
            void showLogo();
        private:
            void _drawFrame();
            void _reverseDirection();
            int _currentFrame;
            int _direction;
    };
}

#endif