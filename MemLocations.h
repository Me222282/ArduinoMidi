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
#define DUPLICATE_NOTES 274

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
#define SEQ_CLOCKED 194
#define SEQ_BAR_TRIGGER 195
#define SEQ_BAR_SIZE 196

#define TRACKS_SLOT_1 66
#define TRACK_CLOCKDIV 0
#define TRACK_USEMOD 1
#define TRACK_HALFTIME 2
#define TRACK_SIZE 3
// repeat 32x

#define SEQS_SLOT_1 197
// repeat 16x

#define VIBRATO_1 213
#define T_RATE_A 0
#define T_RATE_B 1
#define T_RATE_C 2
#define T_RATE_D 3
#define T_SCALE_A 4
#define T_SCALE_B 5
#define T_SCALE_C 6
#define T_SCALE_D 7
#define T_MODE 8
#define T_ENABLED 9
// repeat 5x

#define OFFSETS_1 263
#define OFF_OCT 0
#define OFF_ST 1
// repeat 5x

#define TREM_GLOB 273

#define EEPROM_SIZE 275

#ifdef __cplusplus
} // extern "C"
#endif

template<typename T>
inline void FlashWrite(uint32_t address, const T* value, uint16_t size)
{
    ESP.flashWrite(address, (uint32_t*)value, sizeof(T) * size);
}
template<typename T>
inline T* FlashRead(uint32_t address, uint16_t size)
{
    T* values = CREATE_ARRAY(T, size);
    ESP.flashRead(address, (uint32_t*)values, sizeof(T) * size);
    return values;
}

#endif