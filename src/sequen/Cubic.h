#ifndef __cubes
#define __cubes

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float a;
    float b;
    float c;
    float d;
} Cubic;

typedef struct
{
    float x3;
    float x2;
    float x;
} CubicInput;

CubicInput getInput(float x);
float getCubicValue(Cubic c, CubicInput ci);
Cubic generateCubic(uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4);

#ifdef __cplusplus
} // extern "C"
#endif

#endif