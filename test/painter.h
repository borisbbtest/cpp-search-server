#include "wall.h"
class Painter
{
public:
    // Напишите класс Painter

    void Paint(Wall& wall, Wall::Color& tt) : wall_(wall), color(tt){

    }

private:
    Wall  wall_;
    Wall::Color color_;
    // Напишите класс Painter
};