#ifndef __history
#define __history

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _item
{
    _item* next;
    uint8_t value;
} _hItem;

typedef struct
{
    // count and range solves small problem - user can just reset by changing other parameter
    // uint8_t cap;
    // uint8_t count;
    _hItem* first;
    _hItem* last;
} History;

History allocateHistory(uint8_t range);
void clearHistory(History* h);
void pushHistory(History* h, uint8_t v);
uint8_t pullHistory(History* h);

#ifdef __cplusplus
} // extern "C"
#endif

#endif