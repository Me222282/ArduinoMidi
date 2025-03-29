#ifndef __glob
#define __glob

#define CREATE(type) (type*)calloc(1, sizeof(type));
#define CREATE_ARRAY(type, size) (type*)calloc(size, sizeof(type));

// Pins
// Analoge
#define OCTAVE A0
// #define POT1 A2
// #define POT2 A1
#define MODE1 A3
#define MODE2 A4
#define VOICES A5
#define CHANNELS A6
#define PITCHBEND A7
// Digital
// #define POT3 1
#define NOTEDAC1 2
#define NOTEDAC2 3
#define NOTEDAC3 4
#define NOTEDAC4 5
#define NOTEDAC5 6
#define PITCHDAC3 7
#define PITCHDAC2 8
#define PITCHDAC1 9
#define GATEIC 10
#define OPTIONMODE 12

#define RETRIGTIME 2

#ifndef __nnNote
#define __nnNote
typedef struct
{
    uint8_t key;
    uint8_t vel;
} Note;
#endif

// divisions to make pb smaller
const float pbDiv[6] = { 1.0f / 24.0f, 1.0f / 12.0f, 1.0f / 6.0f, 5.0f / 12.0f, 7.0f / 12.0f, 1.0f };

uint8_t activeChannels = 1;
uint8_t activeVoices = 1;
float pitchBendSelect = pbDiv[1];
// 0 - ignore, 1 - take last, 2 - take first
uint8_t mode2 = 0;
// OFF for loop
bool mode1 = false;

int8_t octaveOffset = 0;
bool option = false;

Note oldValues[5];
uint16_t oldPBs[5];
uint16_t oldMod = 0;

uint8_t gate = 0;
uint8_t reTrig = 0;

void setGateNote(uint8_t n, bool value)
{
    uint8_t g = 1 << n;
    if (value)
    {
        gate |= g;
        return;
    }

    gate &= ~g;
}
void reTrigNote(uint8_t n)
{
    uint8_t g = 1 << n;
    reTrig |= g;
}

#endif