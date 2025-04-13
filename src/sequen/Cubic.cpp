#include <Arduino.h>
#include "Cubic.h"

CubicInput getInput(float x)
{
    float x2 = x * x;
    return { x2 * x, x2, x };
}

float getCubicValue(Cubic c, CubicInput ci)
{
    return (c.a * ci.x3) + (c.b * ci.x2) + (c.c * ci.x) + c.d;
}

Cubic generateCubic(uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4)
{
    float d = (float)p2;
    int32_t b2 = p1 + p3 - p2 - p2;
    float b = ((float)b2) * 0.5f;
    int32_t a6 = p4 + p2 - p3 - p3 - b2;
    float a = ((float)a6) / 6.0f;
    float c = (float)(p3 - p2) - a - b;
    
    return { a, b, c, d };
}