#ifndef __datadatadata
#define __datadatadata

enum DataSpace: uint8_t
{
    Special,
    CCData,
    TTData,
    Tracks,
    Sequens,
    SeqGlobals,
    Arps
};

#ifdef __cplusplus
extern "C" {
#endif

void clearAllNV();

void openDataSpace(DataSpace space, bool rw);
void closeDataSpace();

void setSpaceByte(uint16_t address, uint8_t value);
uint8_t getSpaceByte(uint16_t address);
void setSpaceInt(uint16_t address, uint32_t value);
uint32_t getSpaceInt(uint16_t address);
void setSpaceFloat(uint16_t address, float value);
float getSpaceFloat(uint16_t address);

void setSpaceBlock(uint16_t address, void* bytes, size_t size);
void getSpaceBlock(uint16_t address, void* bytes, size_t size);

void commitSpace();

#ifdef __cplusplus
} // extern "C"
#endif

template<typename T>
inline void setSpaceData(uint32_t address, T value)
{
    setSpaceBlock(address, &value, sizeof(T));
}
template<typename T>
inline T getSpaceData(uint32_t address)
{
    T out;
    getSpaceBlock(address, &out, sizeof(T));
    return out;
}

#endif