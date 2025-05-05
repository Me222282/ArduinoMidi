#include <Arduino.h>
#include <nvs.h>
#include <nvs_flash.h>
#include "NVData.h"

void clearAllNV()
{
    nvs_flash_erase();
}

nvs_handle_t _handle;

void openDataSpace(DataSpace space, bool rw)
{
    // ensure multiple are not open at a time
    if (_handle) { return; }
    
    char n[2] = { (char)(space + 33), '\0' };
    nvs_open(n, rw ? NVS_READWRITE : NVS_READONLY, &_handle);
}
void closeDataSpace()
{
    nvs_close(_handle);
    _handle = 0;
}

uint32_t f_to_i(float v)
{
    uint32_t* ptr = (uint32_t*)&v;
    return *ptr;
}
float i_to_f(uint32_t v)
{
    float* ptr = (float*)&v;
    return *ptr;
}

void setSpaceByte(uint16_t address, uint8_t value)
{
    address = (address << 6) | 0x8020;
    char key[3] = { (char)(address >> 8), (char)(address & 0x00FF), '\0' };
    uint8_t there = 0;
    nvs_get_u8(_handle, key, &there);
    
    if (there == value) { return; }
    nvs_set_u8(_handle, key, value);
}
uint8_t getSpaceByte(uint16_t address)
{
    address = (address << 6) | 0x8020;
    char key[3] = { (char)(address >> 8), (char)(address & 0x00FF), '\0' };
    uint8_t out = 0;
    nvs_get_u8(_handle, key, &out);
    return out;
}
void setSpaceInt(uint16_t address, uint32_t value)
{
    address = (address << 6) | 0x8020;
    char key[3] = { (char)(address >> 8), (char)(address & 0x00FF), '\0' };
    uint32_t there = 0;
    nvs_get_u32(_handle, key, &there);
    
    if (there == value) { return; }
    nvs_set_u32(_handle, key, value);
}
uint32_t getSpaceInt(uint16_t address)
{
    address = (address << 6) | 0x8020;
    char key[3] = { (char)(address >> 8), (char)(address & 0x00FF), '\0' };
    uint32_t out = 0;
    nvs_get_u32(_handle, key, &out);
    return out;
}
void setSpaceFloat(uint16_t address, float value)
{
    uint32_t data = f_to_i(value);
    
    address = (address << 6) | 0x8020;
    char key[3] = { (char)(address >> 8), (char)(address & 0x00FF), '\0' };
    uint32_t there = 0;
    nvs_get_u32(_handle, key, &there);
    
    if (there == data) { return; }
    nvs_set_u32(_handle, key, data);
}
float getSpaceFloat(uint16_t address)
{
    address = (address << 6) | 0x8020;
    char key[3] = { (char)(address >> 8), (char)(address & 0x00FF), '\0' };
    uint32_t out = 0;
    nvs_get_u32(_handle, key, &out);
    return i_to_f(out);
}

void commitSpace()
{
    nvs_commit(_handle);
}

void setSpaceBlock(uint16_t address, void* bytes, size_t size)
{
    address = (address << 6) | 0x8020;
    char key[3] = { (char)(address >> 8), (char)(address & 0x00FF), '\0' };
    nvs_set_blob(_handle, key, bytes, size);
}
void getSpaceBlock(uint16_t address, void* bytes, size_t size)
{
    address = (address << 6) | 0x8020;
    char key[3] = { (char)(address >> 8), (char)(address & 0x00FF), '\0' };
    nvs_get_blob(_handle, key, bytes, &size);
}