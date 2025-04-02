#ifndef __notes
#define __notes

#include "Channels.h"
#include "../core/Globals.h"

extern bool retriggerNew;
extern bool retriggerOld;
extern bool sortNotes;

int8_t pushNote(Channel* c, uint8_t chI, Note n);
int8_t removeNote(Channel* c, uint8_t chI, uint8_t key);

#endif