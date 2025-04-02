#include "History.h"
#include "../core/Globals.h"

History allocateHistory(uint8_t range)
{
    range--;
    
    History h;
    // h.cap = range;
    // h.count = range;
    
    _hItem* current = CREATE(_hItem);
    h.first = current;
    for (uint8_t i = 0; i < range; i++)
    {
        current->value = i;
        _hItem* next = CREATE(_hItem);
        current->next = next;
        current = next;
    }
    current->value = range;
    h.last = current;
    return h;
}
void clearHistory(History* h)
{
    _hItem* current = h->first;
    while (current)
    {
        _hItem* next = current->next;
        free(current);
        current = next;
    }
    h->first = nullptr;
    h->last = nullptr;
    // h->count = 0;
}

void pushHistory(History* h, uint8_t v)
{
    // if (h->count >= h->cap) { return; }
    // h->count++;
    
    _hItem* i = CREATE(_hItem);
    i->value = v;
    
    if (!h->last)
    {
        h->first = i;
        h->last = i;
        return;
    }
    
    h->last->next = i;
    h->last = i;
}
uint8_t pullHistory(History* h)
{
    _hItem* i = h->first;
    if (!i) { return -1; }
    
    // h->count--;
    h->first = i->next;
    
    if (!i->next)
    {
        h->last = nullptr;
    }
    
    uint8_t v = i->value;
    free(i);
    return v;
}