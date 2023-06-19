// Compatible with Teensy 4.1

#include <MIDI.h>

#define STEPS 16
#define ROWS 8
#define PATTERNS 4
#define BARS 16
#define ON 127
#define OFF 0
#define PPQN 24
#define ONE_SECOND 60000000

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

void sendAllNotesOff() {
  // Send all notes off
  for (int i = 1; i <= 16; i++) {
    MIDI.sendControlChange(123, 0, i);
    // exit(0);
  }
}

void setup() {
  Serial.begin(9600);
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

  // Listen to all incoming messages
  MIDI.begin(MIDI_CHANNEL_OMNI);  

  // Send all notes off
  sendAllNotesOff();

  // Initialize synthesizers
  MIDI.sendProgramChange(2, 1);
  MIDI.sendProgramChange(2, 2);
  MIDI.sendProgramChange(2, 3);

  // Initialize MIDI Clock
  byte tick = 0;
  while (tick < 4) {    
    unsigned long now = micros();

    // Check if a pulse length has passed and if so process audio frame
    // Noe that 60000000 micros is 60 seconds, used to calculate the interval for one beat
    // and 24 is the number of pulses per quarter note required for the MIDI Clock
    if (now - previous > ONE_SECOND / bpm / PPQN) {
      Serial.println(now - previous);
      previous = now;
      MIDI.sendClock();
      tick++;
    }
  }

  // Reset previous
  previous = 0;
  
  // Start playhead
  MIDI.sendStart();
}

int bar = 0;
int pulse = 0;
byte step = 0;
byte pattern = 0;
byte lane = 0;

void loop() {
  unsigned long now = micros();

  // Check if a pulse length has passed and if so process audio frame
  // Noe that 60000000 micros is 60 seconds, used to calculate the interval for one beat
  // and 24 is the number of pulses per quarter note required for the MIDI Clock
  if (now - previous > ONE_SECOND / bpm / PPQN) {
    
    // TODO: handle micors() overflow gracefully so timing stays consistent

    // Reset previous
    previous = now;

    // Send MIDI Clock
    MIDI.sendClock();

    if (pulse % 6 == 0) {
      // Play note
      Serial.println(pattern);
      for (byte ch = 1; ch < 2; ch++) {
        if (tracks[lane].patterns[pattern][step]) {    
          MIDI.sendNoteOn(tracks[lane].note, ON, ch);
          MIDI.sendNoteOn(tracks[lane].note, ON, ch + 1);
          MIDI.sendNoteOn(tracks[lane].note, ON, ch + 2);
        } else {
          MIDI.sendNoteOff(tracks[lane].note, OFF, ch);
          MIDI.sendNoteOff(tracks[lane].note, OFF, ch + 1);
          MIDI.sendNoteOff(tracks[lane].note, OFF, ch + 2);
        }
      }

      // Increase step count
      step++;
      if (step >= STEPS) {
        step = 0;
        bar++;

        // Increase bar count
        if (bar >= BARS) {
          bar = 0;
          // Send all notes off
          sendAllNotesOff();
          lane = random(0, 4);
          pattern = random(0, 8);
          MIDI.sendProgramChange(random(0, 64), 1);
          MIDI.sendProgramChange(random(0, 64), 2);
          MIDI.sendProgramChange(random(0, 64), 3);
        }
      }
    }

    // Increase pulse amount
    pulse++;
    if (pulse >= PPQN) {
      pulse = 0;
    }
  }
}