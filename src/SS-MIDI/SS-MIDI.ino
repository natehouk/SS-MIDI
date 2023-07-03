// Compatible with Teensy 4.1

#include <MIDI.h>
#include <Midier.h>
#include <Fsm.h>
#include <Vector.h>

#define DEBUG true
#define PATTERNS 4
#define STEPS 16
#define VOICES 4
#define TRACKS 1
#define ON 100
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
             PANIC,
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

// MIDI Files
const char *tuneList[] = {
  "LOOPDEMO.MID",
  "BANDIT.MID",
  "ELISE.MID",
  "TWINKLE.MID",
  "GANGNAM.MID",
  "FUGUEGM.MID",
  "POPCORN.MID",
  "AIR.MID",
  "PRDANCER.MID",
  "MINUET.MID",
  "FIRERAIN.MID",
  "MOZART.MID",
  "FERNANDO.MID",
  "SONATAC.MID",
  "SKYFALL.MID",
  "XMAS.MID",
  "GBROWN.MID",
  "PROWLER.MID",
  "IPANEMA.mMID",
  "JZBUMBLE.MID",
};

// LED
const int ledPin = LED_BUILTIN;
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
  byte octave;
  midier::Quality quality;
  midier::Note note;
  String steps[PATTERNS];
};
track tracks[TRACKS];

// Initialize note vector
const int ELEMENT_COUNT_MAX = 1024;
midier::Note storage_array[ELEMENT_COUNT_MAX];
Vector<midier::Note> _notes(storage_array);

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
  for (byte channel = 0; channel < 16; channel++) {
    sendAllNotesOff(channel);
  }
}

void onStoppedRunningTransition() {
  log("onStoppedRunningTransition()");
}

void onStoppedPanickedTransition() {
  log("onStoppedPanickedTransition()");
  sendPanic();
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
  sendClock(PANICKED);
  for (byte channel = 0; channel < 16; channel++) {
    sendAllNotesOff(channel);
  }
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
  noInterrupts();
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
          tracks[0].octave = (tracks[0].octave + 1) % 6;
        }
        if (bars % 8 == 0) {
          if (tracks[0].note == midier::Note::A) {
            tracks[0].note = midier::Note::D;
          } else {
            tracks[0].note = midier::Note::A;
          }
        }
        if (bars % 4 == 0) {
          if (tracks[0].quality == midier::Quality::major) {
            tracks[0].quality = midier::Quality::minor;
          } else {
            tracks[0].quality = midier::Quality::major;
          }
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
  interrupts();
}

void pulse() {
  log("pulse()");

  // Advance finite state machine
  sequencer.run_machine();
}

void blinkLED() {
  log("blinkLED()");

  // Blink LED
  if (ledState == LOW) {
    ledState = HIGH;
    blinks++;  // Increase when LED turns on
  } else {
    ledState = LOW;
  }
  digitalWrite(ledPin, ledState);
}

void sendAllNotesOff(byte channel) {
  log("sendAllNotesOff()");

  // Send all notes off
  MIDI.sendControlChange(123, 0, channel);
}

void sendPanic() {
  log("sendPanic()");

  // Panic all notes off on all channels
  for (int channel = 1; channel <= 16; channel++) {
    for (int note = 0; note <= 127; note++) {
      log("sendNoteOff()");
      MIDI.sendNoteOff(note, OFF, channel);
    }
  }
}

void sendInit() {
  MIDI.sendProgramChange(2, 1);
  MIDI.sendProgramChange(2, 2);
  MIDI.sendProgramChange(2, 3);
  delay(1000);
}

void playChord(midier::Note root, midier::Quality quality, byte octave, byte channel) {
  // a list of all seventh chord degrees
  const midier::Degree degrees[] = { 1, 2, 6 };

  // iterate over all the degrees
  for (auto degree : degrees) {
    // find out the interval to be added to the root note for this degree and quality
    midier::Interval interval = midier::triad::interval(quality, degree);

    // calculate the note of this degree
    midier::Note note = root + interval;

    // play the note
    playNote(note, octave, channel);
  }
}

void stopChord(midier::Note root, midier::Quality quality, byte octave, byte channel) {
  // a list of all seventh chord degrees
  const midier::Degree degrees[] = { 1, 3, 5 };

  // iterate over all the degrees
  for (auto degree : degrees) {
    // find out the interval to be added to the root note for this degree and quality
    midier::Interval interval = midier::triad::interval(quality, degree);

    // calculate the note of this degree
    midier::Note note = root + interval;

    // play the note
    stopNote(note, octave, channel);
  }
}

void playNote(midier::Note note, byte octave, byte channel) {
  Serial.println(midier::midi::number(note, octave));
  MIDI.sendNoteOn(midier::midi::number(note, octave), ON, channel);
}

void stopNote(midier::Note note, byte octave, byte channel) {
  Serial.println(midier::midi::number(note, octave));
  MIDI.sendNoteOff(midier::midi::number(note, octave), OFF, channel);
}

void playStep() {
  for (byte track = 0; track < TRACKS; track++) {
    int channel = tracks[track].channel;
    byte pattern = tracks[track].pattern;
    byte octave = tracks[track].octave;
    midier::Quality quality = tracks[track].quality;
    midier::Note note = tracks[track].note;
    char active = tracks[track].steps[pattern][step];
    char previous = tracks[track].steps[pattern][(step - 1) % 16];
    Serial.println(active);
    if (active == '*') {
      log("playNote()");
      playChord(note, quality, octave, channel);
    } else {
      log("stopNote()");
      // stopChord(note, octave, channel);
      sendAllNotesOff(channel);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("setup()");
  randomSeed(analogRead(0));
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  sequencer.add_transition(&stopped, &running, PLAY, &onStoppedRunningTransition);
  sequencer.add_transition(&stopped, &panicked, PANIC, &onStoppedPanickedTransition);
  sequencer.add_transition(&paused, &stopped, STOP, &onPausedStoppedTransition);
  sequencer.add_transition(&paused, &running, PLAY, &onPausedRunningTransition);
  sequencer.add_transition(&running, &stopped, STOP, &onRunningStoppedTransition);
  sequencer.add_transition(&running, &paused, PAUSE, &onRunningPausedTransition);
  sequencer.add_transition(&panicked, &running, PLAY, &onPanickedRunningTransition);

  // Listen to all incoming messages
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Send all notes off on all channels
  for (byte channel = 0; channel < 16; channel++) {
    sendAllNotesOff(channel);
  }

  // Send panic
  // sendPanic();

  // Initialize synthesizers
  // sendInit();

  // Initialize tracks with patterns
  tracks[0] = {
    1,
    2,
    3,
    midier::Quality::maj7,
    midier::Note::A,
    {
      "*---*---*---*---",
      "*-*-*-*-*-*-*-*-",
      "****************",
      "*---*---*---*---",
    },
  };

  // Start master clock using timer interrupt
  clk.begin(pulse, ONE_MINUTE / PPQN / bpm);
}

void loop() {
  delay(5000);
  sequencer.trigger(PLAY);
  delay(55000);
  sequencer.trigger(STOP);
  delay(5000);
  sequencer.trigger(PANIC);
}