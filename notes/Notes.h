#ifndef __notes
#define __notes

#include "Channels.h"
#include "../core/Globals.h"

extern bool retriggerNew = true;
extern bool retriggerOld = false;
extern bool sortNotes = false;

int8_t pushNote(Channel* c, uint8_t chI, Note n);
int8_t removeNote(Channel* c, uint8_t chI, uint8_t key);

#endif