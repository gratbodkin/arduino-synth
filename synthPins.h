#ifndef SYNTHPINS_h
#define SYNTHPINS_h


#define NOTE1 49
#define NOTE2 48
#define NOTE3 47
#define NOTE4 46
#define NOTE5 45
#define NOTE6 44
#define NOTE7 43
#define NOTE8 42
//#define TOGGLEUP 12
//#define TOGGLEDOWN 13

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
#define OSC_FREQ_PIN 1
#define WAVEFORM_PIN 5
#define ATTACK_PIN 4
#define RELEASE_PIN 3
#define DETUNE_PIN 2
#define LFO_WAVEFORM_PIN 6
#define LFO_FREQ_PIN 7
#define LFO_DEPTH_PIN 8
#define CUTOFF_PIN 9



void setupPins()
{
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
}

#endif
