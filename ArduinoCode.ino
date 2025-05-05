#include <SPI.h>
#include "Controls.h"
#include "src/core/Coms.h"
#include "Arpeggio.h"
#include "src/menus/SpeicalOps.h"
#include "src/menus/CCMenu.h"
#include "src/menus/TMenu.h"
#include "src/sequen/Sequencer.h"
#include "src/core/Globals.h"
#include "src/notes/Channels.h"
#include "src/core/Midi.h"
#include "MemLocations.h"
#include "MenuFeedback.h"

void setup()
{
    pinMode(OCTAVE, INPUT);
    pinMode(MODE1, INPUT);
    pinMode(MODE2, INPUT);
    pinMode(VOICES, INPUT);
    pinMode(CHANNELS, INPUT);
    pinMode(PITCHBEND, INPUT);
    pinMode(NOTEDAC1, OUTPUT);
    pinMode(NOTEDAC2, OUTPUT);
    pinMode(NOTEDAC3, OUTPUT);
    pinMode(NOTEDAC4, OUTPUT);
    pinMode(NOTEDAC5, OUTPUT);
    pinMode(PITCHDAC1, OUTPUT);
    pinMode(PITCHDAC2, OUTPUT);
    pinMode(PITCHDAC3, OUTPUT);
    pinMode(GATEIC, OUTPUT);
    pinMode(OPTIONMODE, INPUT);
    
    digitalWrite(NOTEDAC1, HIGH);
    digitalWrite(NOTEDAC2, HIGH);
    digitalWrite(NOTEDAC3, HIGH);
    digitalWrite(NOTEDAC4, HIGH);
    digitalWrite(NOTEDAC5, HIGH);
    digitalWrite(PITCHDAC1, HIGH);
    digitalWrite(PITCHDAC2, HIGH);
    digitalWrite(PITCHDAC3, HIGH);
    digitalWrite(GATEIC, HIGH);
    
    MIDI.begin();
    // Serial.begin(38400);
    // EEPROM.begin(EEPROM_SIZE);
    
    SPI.begin();
    
    // Setup GATEIC
    configureGate();
    
    // load data
    setChannels(1, 1);
    loadOpsState();
    loadCCMState();
    loadSeqState();
    loadTTMState();
}

Menu* _cMenu = nullptr;
NoteName _lastNote;
void readControls()
{
    int8_t oldOO = octaveOffset;
    octaveOffset = readControl(0) - 3;
    
    uint8_t oldC = activeChannels;
    activeChannels = readControl(1) + 1;
    uint8_t oldV = activeVoices;
    activeVoices = readControl(2) + 1;
    
    float oldPBS = pitchBendSelect;
    uint8_t pbI = readControl(3);
    pitchBendSelect = pbDiv[pbI];
    
    bool oldM1 = mode1;
    mode1 = digitalRead(MODE1);
    uint8_t oldM2 = mode2;
    mode2 = readControl(4);
    
    bool oldOp = option;
    option = digitalRead(OPTIONMODE);
    
    bool newChannels = activeChannels != oldC || activeVoices != oldV || oldM1 != mode1;
    if (oldM2 != mode2 || newChannels)
    {
        clearArps();
        if (newChannels)
        {
            setChannels(activeChannels, activeVoices);
        }
        else
        {
            clearNotes(&channels[0]);
            clearNotes(&channels[1]);
            clearNotes(&channels[2]);
            clearNotes(&channels[3]);
            clearNotes(&channels[4]);
        }
        
        gate = 0;
        setGate(0);
        updateAllPBs();
        if (_cMenu)
        {
            _cMenu->onParamChange();
        }
        else
        {
            switch (_lastNote)
            {
                case NoteName::_A0:
                    _cMenu = getSpecialMenu();
                    _cMenu->onOpen();
                    break;
                case NoteName::_B0:
                    _cMenu = getSeqMenu();
                    _cMenu->onOpen();
                    break;
                case NoteName::C1:
                    _cMenu = getCCMenu();
                    _cMenu->onOpen();
                    break;
                case NoteName::_D1:
                    _cMenu = getTMenu();
                    _cMenu->onOpen();
                    break;
                default:
                    _cMenu = nullptr;
                    break;
            }
        }
    }
    else
    {
        if (oldOO != octaveOffset)
        {
            updateAllNotes();
        }
        if (oldPBS != pitchBendSelect)
        {
            updateAllPBs();
        }
        if (oldOp != option)
        {
            updateOtherPorts();
        }
    }
}

void loop()
{
    readControls();
    invokeTremelo();
    
    if (invokeArp) { invokeArps(); }
    
    if (_cMenu)
    {
        if (!_cMenu->active)
        {
            _cMenu = nullptr;
        }
        else
        {
            _cMenu->invoke();
            return;
        }
    }
    
    if (MIDI.read())
    {
        MidiCode type = MIDI.getType();
        uint8_t channel = MIDI.getChannel();
        switch (type)
        {
            case MidiCode::NoteON:
            case MidiCode::NoteOFF:
            {
                uint8_t n = MIDI.getData1();
                uint8_t vel = MIDI.getData2();
                if (type == MidiCode::NoteOFF || vel == 0)
                {
                    if (n == _lastNote) { _lastNote = (NoteName)255U; }
                    if (channel < 5 && channelArps[channel])
                    {
                        arpRemoveNote(channel, n);
                        return;
                    }
                    onNoteOff(channel, n);
                    return;
                }
                
                _lastNote = (NoteName)n;
                if (channel < 5 && channelArps[channel])
                {
                    arpAddNote(channel, { n, vel });
                    return;
                }
                onNoteOn(channel, n, vel);
                return;
            }
            case MidiCode::CC:
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                return;
            case MidiCode::PitchWheel:
                onPitchBend(channel, MIDI.getCombinedData());
                return;
            case MidiCode::TimingClock:
                if (invokeArp && arpClocked) { clockedArp(); }
                return;
            // case MidiCode::Reset:
            //     resetOpsValues();
            //     resetCCMValues();
            //     resetSeqValues();
                
            //     clearArps();
            //     setChannels(activeChannels, activeVoices);
                
            //     gate = 0;
            //     setGate(gate);
            //     return;
        }
    }
}

