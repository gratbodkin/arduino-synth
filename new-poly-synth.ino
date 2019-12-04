#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <Arduino.h>
// Included Header Files
#include <MozziGuts.h>
#include <mozzi_analog.h>
#include <AutoMap.h> // maps unpredictable inputs to a range
#include <IntMap.h>
#include <Oscil.h>
#include <Smooth.h>
#include <mozzi_midi.h>
#include <mozzi_fixmath.h> //Used for MIDI calcs.  See MIDI.h & mozzi_midi.h to see where it's used
#include <ADSR.h>
#include <LowPassFilter.h>
#include <RCpoll.h>
// Oscillator Tables used for output Waveshape
#include <tables/sin2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
// Oscillator Tables used for Low Frequency Oscillator (LFO)
#include <tables/sin512_int8.h>
#include <tables/saw512_int8.h>
#include <tables/triangle512_int8.h>
#include <tables/square_no_alias512_int8.h>
#define CONTROL_RATE 32 // comment out to use default control rate of 64
#define NOTES_PER_SCALE 8
#define NUM_OSCILLATORS 8
#define NUM_CONTROLS 10
#define SIN 0
#define SAW 1
#define SQUARE 2
#define TRIANGLE 3
#define NOTE1 49
#define NOTE2 48
#define NOTE3 47
#define NOTE4 46
#define NOTE5 45
#define NOTE6 44
#define NOTE7 43
#define NOTE8 42
#define TOGGLEUP 12
#define TOGGLEDOWN 13
#define NOTE1LED 30
#define NOTE2LED 31
#define NOTE3LED 32
#define NOTE4LED 33
#define NOTE5LED 37
#define NOTE6LED 36
#define NOTE7LED 35
#define NOTE8LED 34

#define OCTAVE_PIN 0
#define GAIN_PIN 1
#define OSC_FREQ_PIN 2
#define RELEASE_PIN 3
#define ATTACK_PIN 4
#define WAVEFORM_PIN 5
#define LFOWaveForm_PIN 6
#define LFO_FREQ_PIN 7
#define LFO_DEPTH_PIN 8
#define CUTOFF_PIN 9

Oscil < SIN2048_NUM_CELLS, AUDIO_RATE > oscillators[NOTES_PER_SCALE];
Oscil < SIN2048_NUM_CELLS, AUDIO_RATE > oscillators2[NOTES_PER_SCALE];
ADSR < CONTROL_RATE, CONTROL_RATE > oscEnvelopes[NUM_OSCILLATORS];
Oscil < 512, AUDIO_RATE > lfo(SIN512_DATA);
LowPassFilter filters[NOTES_PER_SCALE];

uint8_t resLookup[4] =
{
    180, 190, 35, 190
};

uint8_t triggerValues[8] =
{
    0, 0, 0, 0, 0, 0, 0, 0
};

const int scaleArray[][NOTES_PER_SCALE] =
{
    {
        0, 2, 4, 5, 7, 9, 11, 12
    },
    {
        0, 2, 3, 5, 7, 9, 11, 12
    },
    {
        0, 2, 3, 5, 7, 8, 11, 12
    },
    {
        0, 3, 5, 6, 7, 10, 11, 12
    },
    {
        0, 2, 3, 5, 7, 8, 10, 12
    }
};

uint8_t gain_adsr[8] =
{
    0, 0, 0, 0, 0, 0, 0, 0
};

// -- Global variable declarations ----------------------------------------------------------------------------
boolean notePressedFlag = false;
int inScale = 4;
int inKey = 0;
int newNote = 0;
int curControl = 0;
int waveform = 0;
int waveformCheck = 0;
uint8_t noteBtnValues = 0x00;
uint8_t prevNoteBtnValues = 0x00;
uint8_t gain_lfo1; // Stores gain multiplier contributed by lfo.


const IntMap waveIntMap(0, 1023, 0, 5);
unsigned int octave = 3;
const IntMap kMapOctave(0, 1023, 3, 8);
unsigned int attack_ms; // Attack time, milliseconds.
const IntMap attackIntMap(0, 1023, 28, 1000); // Min value must be large enough to prevent click at note start.
unsigned int release_ms; // Decay time, milliseconds.
const IntMap releaseIntMap(0, 1023, 25, 1000); // Min value must be large enough to prevent click at note end.
char LFOWaveForm;
int LFOwaveformCheck;
const IntMap lfo_waveIntMap(0, 1023, 1, 5); // 1,5 returns 1-2-3-4.
unsigned int lfo_speed;
const IntMap lfo_speedIntMap(0, 1023, 0, 1400); //
unsigned int cutoff;
const IntMap cutoffIntMap(0, 1023, 30, 180); // Valid range 0-255 corresponds to freq 0-8192 (audio rate/2).
uint8_t lfo_depth;
const IntMap lfo_depthIntMap(0, 1023, 1, 255); // LFO depth, as a percent multiplier of cutoff. 1=0% of cutoff, 256=100% of cutoff.
int gain = 255;
const IntMap gainMap(0, 1023, 1, 128);
float detuneAmount = 0; // Store the output waveform selection 1-2-3-4
const IntMap detuneMap(0, 1023, 0, 1000); // returns 1-2-3-4.
uint8_t ledPinValues = 0x00;
const IntMap kMapScale(0, 1023, 0, 5);
const IntMap kMapKeyOffset(0, 1023, 0, 11);
int midiNoteValue;
int freq;
// -----------------------------------------------------------------------------------------
void setup()
{

    Serial.begin(9600);
    startMozzi(CONTROL_RATE); // Start the use of Mozzi with defined CONTROL_RATE
    pinMode(NOTE1, INPUT_PULLUP);
    pinMode(NOTE2, INPUT_PULLUP);
    pinMode(NOTE3, INPUT_PULLUP);
    pinMode(NOTE4, INPUT_PULLUP);
    pinMode(NOTE5, INPUT_PULLUP);
    pinMode(NOTE6, INPUT_PULLUP);
    pinMode(NOTE7, INPUT_PULLUP);
    pinMode(NOTE8, INPUT_PULLUP);
    pinMode(NOTE7, INPUT_PULLUP);
    pinMode(NOTE8, INPUT_PULLUP);
    pinMode(TOGGLEUP, INPUT_PULLUP);
    pinMode(TOGGLEDOWN, INPUT_PULLUP);

    pinMode(NOTE1LED, OUTPUT);
    pinMode(NOTE2LED, OUTPUT);
    pinMode(NOTE3LED, OUTPUT);
    pinMode(NOTE4LED, OUTPUT);
    pinMode(NOTE5LED, OUTPUT);
    pinMode(NOTE6LED, OUTPUT);
    pinMode(NOTE7LED, OUTPUT);
    pinMode(NOTE8LED, OUTPUT);
    for (int i = 0; i < 8; i++)
    {
        oscillators[i] = Oscil < SIN2048_NUM_CELLS, AUDIO_RATE > (SIN2048_DATA);
        oscillators2[i] = Oscil < SIN2048_NUM_CELLS, AUDIO_RATE > (SIN2048_DATA);
        oscEnvelopes[i].setADLevels(200, 200);
        oscEnvelopes[i].setDecayTime(30000);
        oscEnvelopes[i].setSustainTime(32500);
        oscEnvelopes[i].setReleaseLevel(0);
    }
}

void loop()
{
    audioHook(); // Required here
}

uint8_t findMostRecentlyPressedButton(uint8_t inByte)
{
    uint8_t newBtnValues = inByte & (~prevNoteBtnValues);
    uint8_t buttonsBitMask = 0x01;
    int noteNumber = 8;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (newBtnValues & (buttonsBitMask << i))
        {
            noteNumber = i;
            break;
        }
        buttonsBitMask << 1;
    }
    prevNoteBtnValues = inByte;
    return noteNumber;
}

uint8_t reverse_byte(uint8_t a)
{
    return((a & 0x1) << 7) | ((a & 0x2) << 5) |
    ((a & 0x4) << 3) | ((a & 0x8) << 1) |
    ((a & 0x10) >> 1) | ((a & 0x20) >> 3) |
    ((a & 0x40) >> 5) | ((a & 0x80) >> 7);
}

// -----end Setup-----------------------------------------------------------------------------------
void updateControl()
{
    noteBtnValues = (~PINL);
    Serial.println(noteBtnValues, BIN);
    if (noteBtnValues != prevNoteBtnValues)
    {
        newNote = findMostRecentlyPressedButton(noteBtnValues);
        notePressedFlag = true;
    }
    else
    {
        newNote = 8;
    }
    PORTC = reverse_byte(noteBtnValues);
    switch (curControl)
    {
        case 0:
            octave = kMapOctave(mozziAnalogRead(OCTAVE_PIN));
            break;
        case 1:
            octave = kMapOctave(mozziAnalogRead(OSC_FREQ_PIN));
            break;
        case 2:
            gain = gainMap(mozziAnalogRead(GAIN_PIN));
            break;
        case 3:
            attack_ms = attackIntMap(mozziAnalogRead(ATTACK_PIN));
            break;
        case 4:
            release_ms = releaseIntMap(mozziAnalogRead(RELEASE_PIN));
            break;
        case 5:
            LFOwaveformCheck = lfo_waveIntMap(mozziAnalogRead(LFOWaveForm_PIN));
            if (LFOwaveformCheck != LFOWaveForm)
            {
                LFOWaveForm = LFOwaveformCheck;
                 chooseLFOtable();
                //lfo.setTable(lfoWaveforms[LFOwaveformCheck]);
            }
            break;
        case 6:
            lfo_speed = lfo_speedIntMap(mozziAnalogRead(LFO_FREQ_PIN));
            break;
        case 7:
            lfo_depth = lfo_depthIntMap(mozziAnalogRead(LFO_DEPTH_PIN));
            break;
        case 8:
            cutoff = cutoffIntMap(mozziAnalogRead(CUTOFF_PIN));
            break;
        case 9:
        waveform = waveIntMap(mozziAnalogRead(WAVEFORM_PIN));
        chooseTable();
        break;
        default:
            break;
    }
    curControl = curControl > 9? 0:curControl + 1;
    // detuneAmount = detuneAmount / 1000.0;
    // chooseLFOtable();
    // Serial.println(lfo_speed);
    // Serial.println(cutoff - ((cutoff * gain_lfo1)>>8));
    lfo.setFreq((int) lfo_speed);
    uint8_t noteOnBitMask = 0x01;
    for (int i = 0; i < NOTES_PER_SCALE; i++)
    {
        oscEnvelopes[i].setAttackTime(attack_ms);
        oscEnvelopes[i].setReleaseTime(release_ms);
        oscEnvelopes[i].update();
        filters[i].setCutoffFreq(cutoff - ((cutoff * gain_lfo1) >> 8));
        filters[i].setResonance(resLookup[waveform]);
        if ((noteBtnValues & noteOnBitMask) != 0)
        {
            triggerValues[i] = 1;
            if (newNote == i && notePressedFlag)
            {
                midiNoteValue = (12 * octave) + (inKey + scaleArray[inScale][newNote]);
                freq = mtof(midiNoteValue);
                oscillators[newNote].setFreq(freq);
                // Serial.println(freq);
                // oscillators2[newNote].setFreq(freq);
                oscEnvelopes[i].noteOn();
                notePressedFlag = false;
                // 
            }
            // gain_adsr[i] = oscEnvelopes[i].next();
            // Serial.println(gain_adsr[i]);
        }
        else
        {
            triggerValues[i] = 0;
            oscEnvelopes[i].noteOff();
            // gain_adsr[i] = 0;
        }
        gain_adsr[i] = oscEnvelopes[i].next();
        noteOnBitMask <<= 1;
    }
}

void chooseTable()
{
    for (int i = 0; i < NUM_OSCILLATORS; i++)
    {
        switch (waveform)
        {
            case SAW:
                oscillators[i].setTable(SAW2048_DATA);
                // oscillators2[i].setTable(SAW2048_DATA);
                break;
            case SIN:
                oscillators[i].setTable(SIN2048_DATA);
                // oscillators2[i].setTable(SIN2048_DATA);
                break;
            case SQUARE:
                oscillators[i].setTable(SQUARE_NO_ALIAS_2048_DATA);
                // oscillators2[i].setTable(SQUARE_NO_ALIAS_2048_DATA);
                break;
            case TRIANGLE:
                oscillators[i].setTable(TRIANGLE2048_DATA);
                // oscillators2[i].setTable(TRIANGLE2048_DATA);
                break;
        }
    }
}

void chooseLFOtable()
{
    switch (LFOWaveForm)
    {
        case SAW:
            lfo.setTable(SAW512_DATA);
            break;
        case SIN:
            lfo.setTable(SIN512_DATA);
            break;
        case SQUARE:
            lfo.setTable(SQUARE_NO_ALIAS512_DATA);
            break;
        case TRIANGLE:
            lfo.setTable(TRIANGLE512_DATA);
            break;
    }
    // gain_lfo1 = (int)((long)(lfo.next() + 128) * lfo_depth) >> 8;
    // gain_lfo1 = (int)((lfo.next() + 128) * lfo_depth) >> 8;
    gain_lfo1 = (int) (lfo.next() * lfo_depth) >> 8;
}

int updateAudio()
{
    long outputValue = 0;
    uint8_t bMask = 0x01;
    for (int i = 0; i < 8; i++)
    {
        outputValue += (int) (oscillators[i].next() * gain_adsr[i] * (bMask & noteBtnValues)) >> 8;
        // int oscValue = (int)(oscillators[i].next() * gain_adsr[i] * (bMask & noteBtnValues)) >> 8;
        // int oscValue = (int)(oscillators[i].next() * gain_adsr[i] * (bMask & noteBtnValues)) >> 8;
        // outputValue += oscValue;
        // int oscValue = (int)(filters[i].next(oscillators[i].next()) * gain_adsr[i]) >> 8;
        // if (triggerValues[i] != 0)
        // {
        // outputValue += oscValue;
        // }
        bMask <<= 1;
    }
    return(long) (outputValue * gain) >> 8;
}

void switchToggledUp()
{
    inScale = inScale + 1 > 4? 4:inScale + 1;
}

void switchToggledDown()
{
    inScale = inScale - 1 < 0? 0:inScale - 1;
}
