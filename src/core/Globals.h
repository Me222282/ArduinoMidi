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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t key;
    uint8_t vel;
} Note;

// divisions to make pb smaller
extern const float pbDiv[6];

// consts
extern const uint16_t digitPlaces[4];

extern uint8_t activeChannels;
extern uint8_t activeVoices;
extern float pitchBendSelect;
// 0 - ignore, 1 - take last, 2 - take first
extern uint8_t mode2;
// OFF for loop
extern bool mode1;

extern int8_t octaveOffset;
extern bool option;

extern Note oldValues[5];
extern uint16_t oldPBs[5];
extern uint16_t oldMod;

extern uint8_t gate;
extern uint8_t reTrig;

void setGateNote(uint8_t n, bool value);
void reTrigNote(uint8_t n);
void eeWrite(int address, uint8_t v);
inline bool noteEquals(Note a, Note b)
{
    return a.key == b.key && a.vel == b.vel;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif