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
enum Event { PLAY,
             PAUSE,
             STOP };
enum Mode { STOPPED,
            PAUSED,
            RUNNING,
};
State stopped(&onStoppedEnter, &onStoppedState, &onStoppedExit);
State paused(&onPausedEnter, &onPausedState, &onPausedExit);
State running(&onRunningEnter, &onRunningState, &onRunningExit);
Fsm sequencer(&stopped);

const int ledPin = LED_BUILTIN;
const int buttonPin = 7;
int ledState = LOW;

// Use volatile for shared variables
volatile unsigned long blinks = 0;
volatile unsigned int step = 0;

byte bpm = 123;
int measure = 0;
unsigned long previous = 0;

struct track {
  byte note;
  int channel;
  byte pattern;
  bool patterns[PATTERNS][STEPS];
};
track tracks[ROWS];

void log(String message) {
  if (DEBUG) {
    Serial.println(message);
  }
}

void debug(String message) {
  Serial.println(message);
}

void onStoppedEnter() {
  log("onStoppedEnter()");
}

void onStoppedExit() {
  log("onStoppedExit()");
}

void onStoppedState() {
  log("onStoppedState()");
  sendClock(STOPPED);
}

void onStoppedRunningTransition() {
  log("onStoppedRunningTransition()");
}

void onPausedEnter() {
  log("onPausedEnter()");
}

void onPausedState() {
  log("onPausedState()");
  sendClock(PAUSED);
}

void onPausedExit() {
  log("onPausedExit()");
}

void onPausedStoppedTransition() {
  log("onPausedStoppedTransition()");
}

void onPausedRunningTransition() {
  log("onPausedRunningTransition()");
}

void onRunningStoppedTransition() {
  log("onRunningStoppedTransition()");
}

void onRunningEnter() {
  log("onRunningEnter()");
}

void onRunningState() {
  log("onRunningState()");
  sendClock(RUNNING);
  playStep();
}

void onRunningExit() {
  log("onRunningExit()");
}

void onRunningPausedTransition() {
  log("onRunningPausedTransition()");
}

void sendStart() {
  MIDI.sendStart();
}

void sendClock(Mode mode) {
  debug("sendClock()");
  switch (mode) {
    case RUNNING:
      step++;
      if (step % PPQN == 0) {
        step = 0;
        blinkLED();
      }
      break;
    case PAUSED:
      break;
    case STOPPED:
      step = 0;
      break;
  }
  MIDI.sendClock();
}

void pulse() {
  log("pulse()");

  // Check conditions
  if (!random(0, 10)) {
    log("Button pressed");
    sequencer.trigger(PLAY);
  }

  // Advance finite state machine
  sequencer.run_machine();
}

void blinkLED() {
  log("blinkLED()");

  // Blind LED
  if (ledState == LOW) {
    ledState = HIGH;
    blinks++;  // Increase when LED turns on
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

void playStep() {
  if (step % 6 == 0) {
    for (byte track = 0; track < 1; track++) {
      byte pattern = tracks[track].pattern;
      int channel = tracks[track].channel;
      if (tracks[track].patterns[pattern][step]) {
        log("sendNoteOn()");
        MIDI.sendNoteOn(tracks[track].note, ON, channel);
      } else {
        log("sendNoteOff()");
        MIDI.sendNoteOff(tracks[track].note, OFF, channel);
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup()");
  randomSeed(analogRead(0));
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(ledPin, HIGH);
  sequencer.add_transition(&stopped, &running, PLAY, &onStoppedRunningTransition);
  sequencer.add_transition(&paused, &stopped, STOP, &onPausedStoppedTransition);
  sequencer.add_transition(&paused, &running, PLAY, &onPausedRunningTransition);
  sequencer.add_transition(&running, &stopped, STOP, &onRunningStoppedTransition);
  sequencer.add_transition(&running, &paused, PAUSE, &onRunningPausedTransition);

  // Listen to all incoming messages
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Send all notes off
  sendAllNotesOff();

  // Initialize synthesizers
  MIDI.sendProgramChange(2, 1);
  MIDI.sendProgramChange(2, 2);
  MIDI.sendProgramChange(2, 3);

  // Initialize patterns
  tracks[0] = { 36, 1, 0, {
                      { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
                      { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
                      { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
                      { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
                    } };
  tracks[1] = { 40, 2, 0, {
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
                    } };
  tracks[2] = { 42, 3, 0, {
                      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
                      { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 },
                      { 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1 },
                      { 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 },
                    } };
  tracks[3] = { 51, 4, 0, {
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1 },
                      { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0 },
                    } };
  tracks[4] = { 37, 5, 0, {
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    } };
  tracks[5] = { 38, 6, 0, {
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    } };
  tracks[6] = { 75, 7, 0, {
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                    } };
  tracks[7] = { 76, 8, 0, {
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