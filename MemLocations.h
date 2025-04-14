#ifndef __remember
#define __remember

#ifdef __cplusplus
extern "C" {
#endif

#define ARP_TIMEOUT_A 0
#define ARP_TIMEOUT_B 1
#define ARP_TIMEOUT_C 2
#define ARP_TIMEOUT_D 3
#define ARP_MODE 4
#define ARP_SORT 5
#define ARP_HALFTIME 6
// repeat x5

#define RETRIG_OLD 35
#define RETRIG_NEW 36
#define ALWAYS_DELAY 37
#define MICROTONAL 38
#define SORT_NOTES 39
#define ENABLE_ARP_1 40
#define ENABLE_ARP_2 41
#define ENABLE_ARP_3 42
#define ENABLE_ARP_4 43
#define ENABLE_ARP_5 44
#define FILTER_KEYS 45
#define KEY_FILTER 46
#define MIDI_CLOCKED_ARP 47
#define FORGET_NOTES 48

#define ALL_CHANNEL 60
#define FEEDBACK_ENABLED 61

#define CC_OUT_NUMBER_1 49
#define CC_OUT_NUMBER_2 50
#define CC_OUT_NUMBER_3 51
#define CC_OUT_NUMBER_4 52
#define CC_OUT_NUMBER_5 53
#define CC_USE_OUT_1 54
#define CC_USE_OUT_2 55
#define CC_USE_OUT_3 56
#define CC_USE_OUT_4 57
#define CC_USE_OUT_5 58
#define CC_MULTI_CHANNEL 59

#define SEQ_TIMEOUT_A 62
#define SEQ_TIMEOUT_B 63
#define SEQ_TIMEOUT_C 64
#define SEQ_TIMEOUT_D 65

#define TRACKS_SLOT_1 66
#define TRACK_CLOCKDIV 0
#define TRACK_USEMOD 1
#define TRACK_HALFTIME 2
#define TRACK_SIZE 3

#define EEPROM_SIZE 194

#ifdef __cplusplus
} // extern "C"
#endif

#endif