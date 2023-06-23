// Compatible with Teensy 4.1

#include <MIDI.h>
#include <Fsm.h>

#define DEBUG true
#define STEPS 16
#define ROWS 8
#define PATTERNS 4
#define BARS 4
#define PHRASES 4
#define ON 127
#define OFF 0
#define PPQN 24
#define ONE_SECOND 60000000

// Create and bind the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();

// Master clock
IntervalTimer clk;

// Finite state machine
enum event { PLAY, PAUSE, STOP, TICK };
State stopped(&onStopEnter, &onStopState, &onStopExit);
State running(&onRunEnter, &onRunState, &onRunExit);
Fsm sequencer(&stopped);

const int ledPin = LED_BUILTIN;
int ledState = LOW;

// Use volatile for shared variables
volatile unsigned long blinks = 0; 
volatile unsigned int pulses = 0;

byte bpm = 123;
int measure = 0;
unsigned long previous = 0;

struct track {
  byte note;
  bool patterns[PATTERNS][STEPS];
};
track tracks[ROWS];

void log(String message) {
  if (DEBUG) {
    Serial.println(message);
  }
}

void onRunEnter() {
  log("onRunEnter()");
}

void onRunState() {
  log("onRunState()");
  sendClock();
}

void onRunExit() {
  log("onRunExit()");
}

void onRunRunTransition() {
  log("onRunRunTransition()");
}

void onStopEnter() {
  log("onStopEnter()");
}

void onStopExit() {
  log("onStopExit()");
}

void onStopState() {
  log("onStopState()");
  sendClock();
}

void onStopRunTransition() {
  log("onStopRunTransition()");
}

void sendStart() {
  MIDI.sendStart();
}

void sendClock() {
  log("sendClock()");
  pulses++;
  if (pulses % PPQN == 0) {
    pulses = 0;
    blinkLED();
  }
  MIDI.sendClock();
}

void pulse() {
  log("pulse()");

  // Advance finite state machine
  sequencer.run_machine();
}

void blinkLED() {
  log("blindLED()");

  // Blind LED
  if (ledState == LOW) {
    ledState = HIGH;
    blinks++; // Increase when LED turns on
  } else {
    ledState = LOW;
  }
  digitalWrite(ledPin, ledState);
}

void sendAllNotesOff() {
  log("sendAllNotesOff()");

  // Send all notes off to all channels
  for (int i = 1; i <= 16; i++) {
    MIDI.sendControlChange(123, 0, i);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup()");
  randomSeed(analogRead(0));
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  sequencer.add_transition(&stopped, &running, PLAY, &onStopRunTransition);
  sequencer.add_transition(&running, &running, TICK, &onRunRunTransition);

  // Listen to all incoming messages
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Send all notes off
  sendAllNotesOff();

  // Initialize synthesizers
  MIDI.sendProgramChange(2, 1);
  MIDI.sendProgramChange(2, 2);
  MIDI.sendProgramChange(2, 3);

  // Initialize patterns  
  tracks[0] = { 36, {
                      { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
                      { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
                      { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
                      { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
                    } };
  tracks[1] = { 40, {
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
                    } };
  tracks[2] = { 42, {
                      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
                      { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 },
                      { 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1 },
                      { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 },
                    } };
  tracks[3] = { 51, {
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1 },
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0 },
                    } };
  tracks[4] = { 37, {
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    } };
  tracks[5] = { 38, {
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    } };
  tracks[6] = { 75, {
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    } };
  tracks[7] = { 76, {
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
                      { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
                    } };

  // Start master clock
  clk.begin(pulse, ONE_SECOND / PPQN / bpm);
}

void loop() {
}