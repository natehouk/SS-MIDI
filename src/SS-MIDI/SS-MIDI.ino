
#include <MIDI.h>
#include <Fsm.h>
// Compatible with Teensy 4.1

#define PLAY 0
#define TICK 1

#define STEPS 16
#define ROWS 8
#define PATTERNS 4
#define BARS 4
#define PHRASES 4
#define ON 127
#define OFF 0
#define PPQN 24
#define ONE_SECOND 60000000


IntervalTimer clk;

// Create and bind the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();

const int ledPin = LED_BUILTIN;

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
  }
}

State stopped(&onStopEnter, &onStopState, &onStopExit);
State running(&onRunEnter, &onRunState, &onRunExit);
Fsm seq(&stopped);

void onRunEnter() {
  Serial.println("onRunEnter()");
}
void onRunState() {
  Serial.println("onRunState()");
  blinkLED();
  MIDI.sendClock();
}
void onRunExit() {
  Serial.println("onRunExit()");
}
void onRunRunTransition() {
  Serial.println("onRunRunTransition()");
}


void onStopEnter() {
  Serial.println("onStopEnter()");
  blinkLED();
}
void onStopExit() {
  Serial.println("onStopExit()");
}
void onStopState() {
  Serial.println("onStopState()");
  MIDI.sendClock();
}
void onStopRunTransition() {
  Serial.println("onStopRunTransition()");
  MIDI.sendStart();
}

int c = 0;
void pulse() {
  seq.run_machine();
  seq.trigger(TICK);
}



void setup() {
  Serial.begin(9600);
  Serial.println("setup()");
  randomSeed(analogRead(0));
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  seq.add_transition(&stopped, &running, PLAY, &onStopRunTransition);
  seq.add_transition(&running, &running, TICK, &onRunRunTransition);

  // Listen to all incoming messages
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Send all notes off
  sendAllNotesOff();

  // Initialize synthesizers
  MIDI.sendProgramChange(2, 1);
  MIDI.sendProgramChange(2, 2);
  MIDI.sendProgramChange(2, 3);
  
  // tracks[0] = { 36, {
  //                     { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
  //                     { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
  //                     { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
  //                     { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
  //                   } };
  // tracks[1] = { 40, {
  //                     { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
  //                   } };
  // tracks[2] = { 42, {
  //                     { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  //                     { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 },
  //                     { 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1 },
  //                     { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 },
  //                   } };
  // tracks[3] = { 51, {
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1 },
  //                     { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0 },
  //                   } };
  // tracks[4] = { 37, {
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                   } };
  // tracks[5] = { 38, {
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                   } };
  // tracks[6] = { 75, {
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  //                   } };
  // tracks[7] = { 76, {
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
  //                     { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
  //                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
  //                   } };







  // Initialize MIDI Clock
  // byte tick = 0;
  // while (tick < 4) {
  //   unsigned long now = micros();

  //   // Check if a pulse length has passed and if so process audio frame
  //   // Noe that 60000000 micros is 60 seconds, used to calculate the interval for one beat
  //   // and 24 is the number of pulses per quarter note required for the MIDI Clock
  //   if (now - previous > ONE_SECOND / bpm / PPQN) {
  //     Serial.println(now - previous);
  //     previous = now;
  //     
  //     tick++;
  //   }
  // }

  // // Reset previous
  // previous = 0;

  // // Start playhead
  // 
  clk.begin(pulse, 20000);
}


int ledState = LOW;
volatile unsigned long blinkCount = 0; // use volatile for shared variables


void blinkLED() {
  if (ledState == LOW) {
    ledState = HIGH;
    blinkCount = blinkCount + 1;  // increase when LED turns on
  } else {
    ledState = LOW;
  }
  digitalWrite(ledPin, ledState);
}



int phrase = 0;
int bar = 0;
byte step = 0;
byte pattern = 0;
byte lane = 0;




void loop() {
  

  // unsigned long now = micros();

  // // Check if a pulse length has passed and if so process audio frame
  // // Noe that 60000000 micros is 60 seconds, used to calculate the interval for one beat
  // // and 24 is the number of pulses per quarter note required for the MIDI Clock
  // if (now - previous > ONE_SECOND / bpm / PPQN) {

  //   // TODO: handle micros() overflow gracefully so clock stays in time

  //   // Reset previous
  //   previous = now;

  //   // Send MIDI Clock
  //   MIDI.sendClock();

  //   if (pulse % 6 == 0) {
  //     // Play note
  //     Serial.println(pattern);
  //     for (byte ch = 1; ch < 2; ch++) {
  //       if (tracks[lane].patterns[pattern][step]) {
  //         MIDI.sendNoteOn(tracks[lane].note, ON, ch);
  //         MIDI.sendNoteOn(tracks[lane].note + 12, ON, ch + 1);
  //         MIDI.sendNoteOn(tracks[lane].note, ON, ch + 2);
  //       } else {
  //         MIDI.sendNoteOff(tracks[lane].note, OFF, ch);
  //         MIDI.sendNoteOff(tracks[lane].note + 12, OFF, ch + 1);
  //         MIDI.sendNoteOff(tracks[lane].note, OFF, ch + 2);
  //       }
  //     }

  //     // Increase step count
  //     step++;
  //     if (step >= STEPS) {
  //       step = 0;
  //       bar++;

  //       // Increase bar count
  //       if (bar >= BARS) {
  //         bar = 0;
  //         phrase++;

  //         // Send all notes off
  //         sendAllNotesOff();

  //         lane = random(0, 8);
  //         pattern = random(0, 4);
  //         if (phrase >= PHRASES) {
  //           phrase = 0;
  //           if (random(0, 2)) {
  //             MIDI.sendProgramChange(random(0, 127), 1);
  //           }
  //           if (random(0, 2)) {
  //             MIDI.sendProgramChange(random(0, 127), 2);
  //           }
  //           if (random(0, 2)) {
  //             MIDI.sendProgramChange(random(0, 127), 3);
  //           }
  //         }
  //       }
  //     }
  //   }

  //   // Increase pulse amount
  //   pulse++;
  //   if (pulse >= PPQN) {
  //     pulse = 0;
  //   }
  // }
}