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
  byte notes[PATTERNS][STEPS][VOICES];
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
  sendAllNotesOff();
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
  sendAllNotesOff();
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

void sendAllNotesOff() {
  log("sendAllNotesOff()");

  // Send all notes off to all channels
  for (int i = 1; i <= 16; i++) {
    MIDI.sendControlChange(123, 0, i);
  }
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

void playNote(midier::Note note, bool on) {
  // play the note
  // midier::midi::play(note);
  Serial.println(midier::midi::number(note, 2));
  if (on) {
    MIDI.sendNoteOn(midier::midi::number(note, 2), ON, 1);
  } else {
    MIDI.sendNoteOn(midier::midi::number(note, 2), OFF, 1);
  }
}
midier::Note _previousNote;
midier::Note _note;
int test = 0;
void playStep() {
  for (byte track = 0; track < TRACKS; track++) {
    byte pattern = tracks[track].pattern;
    int channel = tracks[track].channel;
    for (byte voice = 0; voice < VOICES; voice++) {
      byte previousStep = (step - 1) % 16;
      byte previousNote = tracks[track].notes[pattern][previousStep][voice];  
      byte note = tracks[track].notes[pattern][step][voice];      
      if (test != 0) {
        _notes.remove(0);
        if (_notes.size() == 0) {
          noteInit();
        }
      }
      _previousNote = _note;
      _note = _notes.front();
      test++;
      if (note == OFF || note != previousNote) {
        log("sendNoteOff()");
        // MIDI.sendNoteOff(previousNote, OFF, channel);
        playNote(_previousNote, false);
      }
      if (note != OFF && note != previousNote) {
        log("sendNoteOn()");
        // MIDI.sendNoteOn(note, ON, channel);
        playNote(_note, true);
      }
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

  // Send all notes off
  sendAllNotesOff();

  // Send panic
  sendPanic();

  // Initialize synthesizers
  sendInit();

  // Initialize tracks with patterns
  tracks[0] = {
    1,
    1,
    {
      { { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 } },
      { { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 48, 0, 0, 0 }, { 48, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 72, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 72, 0, 0, 0 }, { 72, 0, 0, 0 } },
      { { 60, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 60, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 60, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 60, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
      { { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 }, { 60, 0, 0, 0 } },
    },
  };

  // Initialize notes
  noteInit();

  // Start master clock using timer interrupt
  clk.begin(pulse, ONE_MINUTE / PPQN / bpm);
}

void noteInit() {
  const midier::Note notes[] = {
    midier::Note::C,
    midier::Note::D,
    midier::Note::E,
    midier::Note::F,
    midier::Note::G,
    midier::Note::A,
    midier::Note::B,
  };

  // iterate over all the root notes
  for (auto root : notes) {
    // have a list of all seventh chord qualities
    const midier::Quality qualities[] = {
      midier::Quality::m7b5,
      midier::Quality::m7,
      midier::Quality::dom7,
      midier::Quality::maj7,
      midier::Quality::aug7,
    };

    // iterate over all the qualities
    for (auto quality : qualities) {
      // a list of all seventh chord degrees
      const midier::Degree degrees[] = { 1, 3, 5, 7 };

      // iterate over all the degrees
      for (auto degree : degrees) {
        // find out the interval to be added to the root note for this degree and quality
        midier::Interval interval = midier::triad::interval(quality, degree);

        // calculate the note of this degree
        midier::Note note = root + interval;

        // play the note
        _notes.push_back(note);
      }
    }
  }
}

void loop() {
  delay(5000);
  sequencer.trigger(PLAY);
  delay(55000);
  sequencer.trigger(STOP);
  delay(5000);
  sequencer.trigger(PANIC);
}