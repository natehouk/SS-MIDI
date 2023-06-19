#include <MIDI.h>

#define STEPS 16
#define ROWS 8
#define PATTERNS 4
#define ON 127
#define OFF 0
#define PPQN 24

// Create and bind the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();

struct track {
  byte note;
  bool patterns[PATTERNS][STEPS];
};
track tracks[ROWS];

byte bpm = 123;
int measure = 0;
unsigned long previous = 0;

void setup() {
  tracks[0] = { 36, {
    { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
  }};
  tracks[1] = { 40, {
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
  }};
  tracks[2] = { 42, {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1 },
    { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 },
  }};
  tracks[3] = { 51, {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1 },
    { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0 },
  }};
  tracks[4] = { 37, {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  }};
  tracks[5] = { 38, {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  }};
  tracks[6] = { 75, {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  }};
  tracks[7] = { 76, {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
  }};
  MIDI.begin(MIDI_CHANNEL_OMNI);  // Listen to all incoming messages
  for (int i = 1; i <= 16; i++) {
    MIDI.sendControlChange(123, 0, i);
    // exit(0);
  }

  // initialize clock
  byte tick = 0;
  while (tick < 4) {    
    unsigned long now = micros();

    // 60000 is 60 seconds, used to calculate the interval for one beat
    // 24 is the number of pulses per second required for the MIDI Clock
    if (now - previous > 60000000 / bpm / PPQN) {
      Serial.println(now - previous);
      previous = now;
      MIDI.sendClock();
      tick++;
    }
  }
  previous = 0;

  MIDI.sendStart();
}



int pulse = 0;
byte step = 0;
byte pattern = 3;

void loop() {
  unsigned long now = micros();

  // 60000 is 60 seconds, used to calculate the interval for one beat
  // 24 is the number of pulses per second required for the MIDI Clock
  if (now - previous > 60000000 / bpm / PPQN) {
    Serial.println(now - previous);
    previous = now;
    MIDI.sendClock();

    if (pulse % 6 == 0) {
      //play note
      for (byte ch = 0; ch < 1; ch++) {
        if (tracks[ch].patterns[pattern][step]) {    
          MIDI.sendNoteOn(tracks[ch].note, ON, ch + 1);
          MIDI.sendNoteOn(tracks[ch].note + 12, ON, ch + 2);
        } else {
          MIDI.sendNoteOff(tracks[ch].note, OFF, ch + 1);
          MIDI.sendNoteOff(tracks[ch].note + 12, OFF, ch + 2);
        }
      }

      //increase step amount
      step++;
      if (step >= STEPS) {
        step = 0;
      }
    }

    //increase pulse amount
    pulse++;
    if (pulse >= PPQN) {
      pulse = 0;
    }
  }
}