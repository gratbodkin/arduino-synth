#ifndef SCALES_h
#define SCALES_h

#define NOTES_PER_SCALE 8

const int scaleArray[][NOTES_PER_SCALE] = {{0, 2, 4, 5, 7, 9, 11, 12}, {0, 2, 3, 5, 7, 9, 11, 12}, {0, 2, 3, 5, 7, 8, 11, 12}, {0, 3, 5, 6, 7, 10, 11, 12}, {0, 2, 3, 5, 7, 8, 10, 12}};

unsigned int pitch_array[4][13] = {
  {65,   69,  73,  78,  82,  87,  92,  98, 104, 110, 117, 123, 131},
  {131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262},
  {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523},
  {523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1046}
};

#endif