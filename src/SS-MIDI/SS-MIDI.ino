// Compatible with Teensy 4.1

#include <MIDI.h>
#include <Fsm.h>

#define DEBUG true
#define PATTERNS 4
#define STEPS 16
#define VOICES 4

#define ROWS 8

#define BARS 4
#define PHRASES 4

#define ON 127
#define OFF 0
#define PPQN 24
#define ONE_MINUTE 60000000

// Create and bind the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();

// Master clock
IntervalTimer clk;

// Finite state machine
enum Event { PAUSE,
             STOP,
             PLAY,
             PANICK,
};
enum Mode { STOPPED,
            PAUSED,
            RUNNING,
            PANICKED,
};
State stopped(&onStoppedEnter, &onStoppedState, &onStoppedExit);
State paused(&onPausedEnter, &onPausedState, &onPausedExit);
State running(&onRunningEnter, &onRunningState, &onRunningExit);
State panicked(&onPanickedEnter, &onPanickedState, &onPanickedExit);
Fsm sequencer(&stopped);

const int ledPin = LED_BUILTIN;
const int buttonPin = 7;
int ledState = LOW;

// Beats per minute
byte bpm = 123;

// Use volatile for shared variables
volatile unsigned long blinks = 0;
volatile unsigned long pulses = 0;
volatile unsigned long bars = 0;
volatile unsigned long step = 0;

// Track structure
struct track {
  byte channel;
  byte pattern;
  byte notes[PATTERNS][STEPS][VOICES];
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
  sendAllNotesOff();
}

void onStoppedRunningTransition() {
  log("onStoppedRunningTransition()");
}

void onStoppedPanickedTransition() {
  log("onStoppedPanickedTransition()");
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

void onPanickedEnter() {
  log("onPanickedEnter()");
}

void onPanickedExit() {
  log("onPanickedExit()");
}

void onPanickedState() {
  log("onPanickedState()");
  sendClock(STOPPED);
  sendPanick();
}

void onPanickedRunningTransition() {
  log("onPanickedRunningTransition()");
}

void sendStart() {
  // Send MIDI start
  MIDI.sendStart();
}

void sendClock(Mode mode) {
  debug("sendClock()");
  switch (mode) {
    case RUNNING:
      // Increase pulse count
      pulses++;

      // Every 24 pulses equals a 1/4 note
      if (pulses % PPQN == 0) {
        // Reset current pulse
        pulses = 0;

        // Increase bar count
        bars++;
        if (bars % 16 == 0) {
          // #TODO
        }
      }
      
      // Every 12 pulses equals a 1/8 note
      if (pulses % 12 == 0) {
        // Blink at BPM tempo
        blinkLED();
      }

      // Every 6 pulses equals a 1/16 note or one step
      if (pulses % 6 == 0) {
        // Increase current step
        step++;
        if (step % 16 == 0) {
          step = 0;
        }
      }
      break;
    case PAUSED:
      break;
    case STOPPED:
      // Reset current step
      step = 0;
      break;
    case PANICKED:
      // Reset current step
      step = 0;
      break;
  }

  // Send MIDI clock
  MIDI.sendClock();
}

void pulse() {
  log("pulse()");

  // // Check conditions
  // if (!random(0, 10)) {
  //   log("Button pressed");
  //   sequencer.trigger(PLAY);
  // }

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

void sendPanick() {
  log("sendPanick()");

  // Panick all notes off on all channels
  for (int channel = 1; channel <= 16; channel++) {
    for (int note = 0; note < 128; note++) {
      log("sendNoteOff()");
      MIDI.sendNoteOff(note, OFF, channel);
    }
  }
}

void playStep() {
  for (byte track = 0; track < 1; track++) {
    byte pattern = tracks[track].pattern;
    int channel = tracks[track].channel;
    if (tracks[track].patterns[pattern][step]) {
      for (byte voice = 0; voice < 4; voice++) {
        byte note = tracks[track].notes[pattern][step][voice];
        if (voice == 0 || note != 0) {
          log("sendNoteOn()");
          MIDI.sendNoteOn(note, ON, channel);
        }
      }
    }
    else {
      for (byte voice = 0; voice < 4; voice++) {
        byte note = tracks[track].notes[pattern][step][voice];
        if (note != 0) {
          log("sendNoteOff()");
          MIDI.sendNoteOff(note, OFF, channel);
        }
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
  sequencer.add_transition(&stopped, &panicked, PANICK, &onStoppedPanickedTransition);
  sequencer.add_transition(&paused, &stopped, STOP, &onPausedStoppedTransition);
  sequencer.add_transition(&paused, &running, PLAY, &onPausedRunningTransition);
  sequencer.add_transition(&running, &stopped, STOP, &onRunningStoppedTransition);
  sequencer.add_transition(&running, &paused, PAUSE, &onRunningPausedTransition);
  sequencer.add_transition(&panicked, &running, PLAY, &onPanickedRunningTransition);

  // Listen to all incoming messages
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Send all notes off
  sendAllNotesOff();

  // Send panick
  sendPanick();

  // Initialize synthesizers
  MIDI.sendProgramChange(2, 1);
  MIDI.sendProgramChange(2, 2);
  MIDI.sendProgramChange(2, 3);

  // Initialize patterns
  tracks[0] = { 1,
                1,
                {
                   { { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 } },
                   { { 60, 0, 0, 0 }, { 48, 0, 0, 0 }, { 48, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 } },
                   { { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 } },
                   { { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 } },
                },
                {
                  { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 },
                  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
                  { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
                  { 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 },
                }
              };

  // Start master clock
  clk.begin(pulse, ONE_MINUTE / PPQN / bpm);
}

void loop() {
  delay(5000);
  sequencer.trigger(PLAY);
  delay(55000);
  sequencer.trigger(STOP);
}