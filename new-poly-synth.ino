
#include <MozziGuts.h>
#include <Oscil.h> // oscillator 
#include <tables/cos2048_int8.h> // table for Oscils to play
#include <tables/sin2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>
#include <Smooth.h>
#include <AutoMap.h> // maps unpredictable inputs to a range
#include <DCfilter.h>
#include <mozzi_midi.h>
#include <ADSR.h>
#include <EventDelay.h>
#include <Ead.h>
#include <Smooth.h>
#include <LowPassFilter.h>
#include <StateVariable.h>
#include <AudioDelayFeedback.h>
#include <AudioDelay.h>
// desired carrier frequency max and min, for AutoMap
const int MIN_CARRIER_FREQ = 22;
const int MAX_CARRIER_FREQ = 440;
// desired intensity max and min, for AutoMap, note they're inverted for reverse dynamics
const int MIN_INTENSITY = 700;
const int MAX_INTENSITY = 10;
// desired mod speed max and min, for AutoMap, note they're inverted for reverse dynamics
const int MIN_MOD_SPEED = 10000;
const int MAX_MOD_SPEED = 1;
//***********  Param Value Maps **********
AutoMap kMapIntensity(0,1023,MAX_INTENSITY,MIN_INTENSITY);
AutoMap kMapModSpeed(0,1023,MAX_MOD_SPEED,MIN_MOD_SPEED);
AutoMap kRatioMap(0,1023,0,5);
AutoMap kFreqShiftMap(0,1023,0,12);
AutoMap kMapWaveform(0,1023,0,3);
AutoMap kMapOctave(0,1023,3,6);
AutoMap kGainMap(0,1023,0,255);
AutoMap kAttackTimeMap(0,1023,40,2500);
AutoMap kDecayTimeMap(0,1023,58,2500);
AutoMap kDelayTimeMap(0,1023,1600,16384);
AutoMap kDelayFeedbackMap(0,1023,-128,127);
AutoMap kDelayWetMap(0,1023,8,1);
//***********  Knob Pin Defs **********
#define KNOB_PIN 0
#define GAIN_PIN 1
#define FM_INTESNSITY_PIN 2
#define MOD_FREQ_PIN 3
#define RATIO_PIN 4
#define WAVEFORM_PIN 5
#define LFO_WAVEFORM_PIN 6
#define FREQ_SHIFT_PIN 7
#define ATTACK_PIN 8
#define DECAY_PIN 9
#define CUTOFF_PIN 9
//***********  Note Trigger Pin Defs **********
#define NOTE1 49
#define NOTE2 48
#define NOTE3 47
#define NOTE4 46
#define NOTE5 45
#define NOTE6 44
#define NOTE7 43
#define NOTE8 42
//***********  3 Position Switch Pin Defs **********
#define TOGGLEUP 53
// #define TOGGLEDOWN 52
#define SHIFT_FN_PIN 52
#define SHIFT_BTN 2
//***********  Trigger Btn LED Pin Defs **********
#define NOTE1LED 30
#define NOTE2LED 31
#define NOTE3LED 32
#define NOTE4LED 33
#define NOTE5LED 37
#define NOTE6LED 36
#define NOTE7LED 35
#define NOTE8LED 34
#define STACK_SIZE 8
#define CONTROL_RATE 64 // powers of 2 please
#define SIN 0
#define SAW 1
#define SQUARE 2
#define TRIANGLE 3
#define NOTES_PER_SCALE 8
// unsigned int duration, attack, decay, sustain, release_ms;
unsigned int ADSRValues[4] = {50, 500, 1000, 1000};
Smooth <unsigned int> kSmoothFreq(0.85f);
// int lfoWaveform = 0;
int midiNoteValue;
int octave = 3;
int newNote = 0;
int curNote = 0;
int curScale = 4;
int inKey = 0;
int freqShift = 0;
int rootNote = 0;
int prevNoteNumber = 0;
int curStackIndex = 0;
boolean shiftActive = false;
Q16n16 deltime;
int delayWet = 1;
int noteStack[STACK_SIZE] = 
{
    0, 0, 0, 0, 0, 0, 0, 0
};
uint8_t noteBtnValues = 0x00;
uint8_t switchValues = 0x00;
uint8_t prevSwitchValues = 0x00;
uint8_t prevNoteBtnValues = 0x00;
uint8_t triggerValues[8] =
{
    0, 0, 0, 0, 0, 0, 0, 0
};

const int scaleArray[][8] =
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
    },
    {
        -12, -9, -7, -4, 0, 3, 7, 12
    },
    {
        -16, -14, -12, -4, 0, 3, 7, 8
    }
};
int gain;
uint8_t outGain;
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kIntensityMod(COS2048_DATA);
ADSR <CONTROL_RATE, CONTROL_RATE> envelopeAmp;
// AudioDelayFeedback <128> aDel;
AudioDelay <128> aDel;
byte ampEnvGain = 0;
int carrier_freq;
int mod_ratio = 5; // brightness (harmonics)
long fm_intensity; // carries control info from updateControl to updateAudio
// smoothing for intensity to remove clicks on transitions
float smoothness = 0.95f;
Smooth <long> aSmoothIntensity(smoothness);


void setup(){
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
    pinMode(SHIFT_FN_PIN, INPUT_PULLUP);
    pinMode(NOTE1LED, OUTPUT);
    pinMode(NOTE2LED, OUTPUT);
    pinMode(NOTE3LED, OUTPUT);
    pinMode(NOTE4LED, OUTPUT);
    pinMode(NOTE5LED, OUTPUT);
    pinMode(NOTE6LED, OUTPUT);
    pinMode(NOTE7LED, OUTPUT);
    pinMode(NOTE8LED, OUTPUT);
    Serial.begin(9600); // for Teensy 3.1, beware printout can cause glitches
    envelopeAmp.setADLevels(255, 200);
    envelopeAmp.setDecayTime(100);
    envelopeAmp.setSustainTime(32500);
    aModulator.setFreq(0);
    aCarrier.setFreq(0); 
    startMozzi(CONTROL_RATE); // :))
}


void updateControl(){
  noteBtnValues = (~PINL);
  switchValues = (~PINB & 0b11);
  if(prevSwitchValues != switchValues)
  {
    if(isOn(switchValues, SHIFT_BTN))
    {
      shiftActive = true;
    }
    else
    {
      incrementScale();
      shiftActive = false;
    }
    //chooseScale(switchValues);
  }
  prevSwitchValues = switchValues;
  rootNote = inKey + freqShift;
  outGain = kGainMap(mozziAnalogRead(GAIN_PIN));
  if (noteBtnValues != prevNoteBtnValues)
  {
      newNote = findMostRecentlyPressedButton(noteBtnValues);
      if(newNote < 8)
      {
        curNote = newNote;
        envelopeAmp.noteOn();
      }
      else
      {
        envelopeAmp.noteOff();
      }
  }
  midiNoteValue = (12 * octave) + (rootNote + scaleArray[curScale][curNote]);
  carrier_freq = mtof(midiNoteValue);
  aCarrier.setFreq(carrier_freq); 
  outGain = kGainMap(mozziAnalogRead(GAIN_PIN));
  envelopeAmp.setAttackTime(kAttackTimeMap(mozziAnalogRead(ATTACK_PIN)));
  envelopeAmp.setReleaseTime(kDecayTimeMap(mozziAnalogRead(DECAY_PIN)));
  envelopeAmp.update();
  ampEnvGain = (envelopeAmp.next() * outGain) >> 8; 
  PORTC = reverse_byte(noteBtnValues); // Note Trigger Buttons
  octave = kMapOctave(mozziAnalogRead(KNOB_PIN));
  mod_ratio = kRatioMap(mozziAnalogRead(RATIO_PIN));
  int mod_freq = carrier_freq * mod_ratio;
  aModulator.setFreq(mod_freq);
  int LDR1_calibrated = kMapIntensity(mozziAnalogRead(FM_INTESNSITY_PIN));
  fm_intensity = ((long)LDR1_calibrated * (kIntensityMod.next()+128))>>8; // shift back to range after 8 bit multiply
  int LDR2_value= mozziAnalogRead(MOD_FREQ_PIN); // value is 0-1023
  float mod_speed = (float)kMapModSpeed(LDR2_value)/1000;
  kIntensityMod.setFreq(mod_speed);
  if(shiftActive)
  {
    // int feedbackLevel = kDelayFeedbackMap(mozziAnalogRead(LFO_WAVEFORM_PIN));
    // aDel.setFeedbackLevel(feedbackLevel); // can be -128 to 127
    // // deltime = kDelayTimeMap(mozziAnalogRead(FREQ_SHIFT_PIN));
    // deltime = mozziAnalogRead(FREQ_SHIFT_PIN);
    // aDel.setDelayTimeCells(deltime);
    // delayWet =  kDelayWetMap(mozziAnalogRead(WAVEFORM_PIN));
    // int feedbackLevel = kDelayFeedbackMap(mozziAnalogRead(LFO_WAVEFORM_PIN));
    // aDel.setFeedbackLevel(feedbackLevel); // can be -128 to 127
    deltime = kDelayTimeMap(mozziAnalogRead(FREQ_SHIFT_PIN));
    //deltime = mozziAnalogRead(FREQ_SHIFT_PIN);
    aDel.set(deltime);
    Serial.println(deltime);
    // delayWet =  kDelayWetMap(mozziAnalogRead(WAVEFORM_PIN));
  }
  else
  {
    freqShift = kFreqShiftMap(mozziAnalogRead(FREQ_SHIFT_PIN));
    int lfoWaveform = kMapWaveform(mozziAnalogRead(LFO_WAVEFORM_PIN));
    int waveform = kMapWaveform(mozziAnalogRead(WAVEFORM_PIN));
    chooseLFOTable(lfoWaveform);
    chooseTable(waveform);
  }
}


int updateAudio(){
  long modulation = aSmoothIntensity.next(fm_intensity) * aModulator.next();
  int outSignal = (aCarrier.phMod(modulation) * ampEnvGain) >> 8;
  int delaySignal = (int)aDel.next(outSignal, deltime); 
 // int outSignal = (((aCarrier.phMod(modulation)/(8 - (delayWet + 1))) + (aDel.next()/delayWet)) * ampEnvGain) >> 8;
  //return asig/8 + aDel.next(asig, deltime); // mix some straight signal with the delayed signal
   // if(envelopeAmp.playing() == false)
   // {
   //    outSignal = -244;
   // }  
  // return outSignal;
  // return (outSignal + delayWetSignal) >> 1;
  return delaySignal;
}


void loop(){
  audioHook();
}

void chooseTable(int wave)
{
        switch (wave)
        {
            case SAW:
                aCarrier.setTable(SAW2048_DATA);
                break;
            case SIN:
                aCarrier.setTable(SIN2048_DATA);
                break;
            case SQUARE:
                aCarrier.setTable(SQUARE_NO_ALIAS_2048_DATA);
                break;
            case TRIANGLE:
                aCarrier.setTable(TRIANGLE2048_DATA);
                break;
        }
}

void chooseLFOTable(int wave)
{
        switch (wave)
        {
            case SAW:
                kIntensityMod.setTable(SAW2048_DATA);
                break;
            case SIN:
                kIntensityMod.setTable(SIN2048_DATA);
                break;
            case SQUARE:
                kIntensityMod.setTable(SQUARE_NO_ALIAS_2048_DATA);
                break;
            case TRIANGLE:
                kIntensityMod.setTable(TRIANGLE2048_DATA);
                break;
        }
}

void incrementScale()
{
  Serial.println("CHOOSE SCALE");
  int newScale = curScale + 1;
  int numScales = (sizeof(scaleArray)/sizeof(int))/NOTES_PER_SCALE;
  curScale = newScale == numScales ? 0 : newScale;
}

void chooseScale(int inUpOrDown)
{
  int changeAmt = inUpOrDown == 2 ? 1 : inUpOrDown == 1 ? -1 : 0;
  int newScale = curScale + changeAmt;
  int numScales = (sizeof(scaleArray)/sizeof(int))/NOTES_PER_SCALE;
  int maxScaleIndex = numScales - 1;
  curScale = newScale > maxScaleIndex ? maxScaleIndex : newScale < 0 ? 0 : newScale;
}

uint8_t findMostRecentlyPressedButton(uint8_t inByte)
{
    uint8_t newBtnValues = inByte & (~prevNoteBtnValues);
    uint8_t buttonsBitMask = 0x01;
    int noteNumber = 8;
    for (uint8_t i = 0; i < 8; i++)
    {
      if(newBtnValues == 0)
      {
          if ((inByte & (buttonsBitMask << i)) != 0)
        {
          noteNumber = i;

            break;
        }
      }
      else
      {
        if ((newBtnValues & (buttonsBitMask << i)) != 0)
        {
            noteNumber = i;
            break;
        }
      }
        buttonsBitMask << 1;
    }
    
    prevNoteBtnValues = inByte;
    return noteNumber;
}

boolean isOn(uint8_t inPortValue, uint8_t inPinMask)
{
  return (inPortValue * inPinMask) > 0;
}

void push(int inNoteNum)
{
  if(curStackIndex < STACK_SIZE - 1)
  {
    curStackIndex++;
    noteStack[curStackIndex] = inNoteNum;
  }
}

int pop()
{
  if(curStackIndex > -1)
  {
    curStackIndex--;
    noteStack[curStackIndex] = curStackIndex;
  }
  return curStackIndex;
}

uint8_t reverse_byte(uint8_t a)
{
    return((a & 0x1) << 7) | ((a & 0x2) << 5) |
    ((a & 0x4) << 3) | ((a & 0x8) << 1) |
    ((a & 0x10) >> 1) | ((a & 0x20) >> 3) |
    ((a & 0x40) >> 5) | ((a & 0x80) >> 7);
}






