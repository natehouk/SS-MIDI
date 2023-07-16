// Compatible with Teensy 4.1

#include <MIDI.h>
#include <Midier.h>
#include <Fsm.h>
#include <Vector.h>
#include <SD.h>
#include <SPI.h>

#define DEBUG true
#define PATTERNS 4
#define STEPS 16
#define VOICES 4
#define TRACKS 1
#define ON 100
#define OFF 0
#define PPQN 24
#define ONE_MINUTE 60000000

Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 10;

const int debounceDelay = 10;  // iterations to wait until pin is stable
bool lastPlayButtonState = LOW;  // Last state of the button
int count;   // add this variable to store the number of presses

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
Mode mode = Mode::STOPPED;
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
const int runLedPin = 6;
const int playButtonPin = 5;
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
  sendClock();
  for (byte channel = 0; channel < 16; channel++) {
    sendAllNotesOff(channel);
  }
}

void onStoppedRunningTransition() {
  log("onStoppedRunningTransition()");
  mode = Mode::RUNNING;
}

void onStoppedPanickedTransition() {
  log("onStoppedPanickedTransition()");
  mode = Mode::PANICKED;
  sendPanic();
}

void onPausedEnter() {
  log("onPausedEnter()");
}

void onPausedState() {
  log("onPausedState()");
  sendClock();
}

void onPausedExit() {
  log("onPausedExit()");
}

void onPausedStoppedTransition() {
  log("onPausedStoppedTransition()");
  mode = Mode::STOPPED;
}

void onPausedRunningTransition() {
  log("onPausedRunningTransition()");
  mode = Mode::RUNNING;
}

void onRunningStoppedTransition() {
  log("onRunningStoppedTransition()");
  mode = Mode::STOPPED;
}

void onRunningEnter() {
  log("onRunningEnter()");
}

void onRunningState() {
  log("onRunningState()");
  sendClock();
  playStep();
}

void onRunningExit() {
  log("onRunningExit()");
}

void onRunningPausedTransition() {
  log("onRunningPausedTransition()");
  mode = Mode::PAUSED;
}

void onPanickedEnter() {
  log("onPanickedEnter()");
}

void onPanickedExit() {
  log("onPanickedExit()");
}

void onPanickedState() {
  log("onPanickedState()");
  sendClock();
  for (byte channel = 0; channel < 16; channel++) {
    sendAllNotesOff(channel);
  }
}

void onPanickedRunningTransition() {
  log("onPanickedRunningTransition()");
  mode = Mode::RUNNING;
}

void sendStart() {
  // Send MIDI start
  MIDI.sendStart();
}

void sendClock() {
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
        if (step % 2 == 0) {
          if (tracks[0].note == midier::Note::C) {
            tracks[0].note = midier::Note::D;
          } else if (tracks[0].note == midier::Note::D) {
            tracks[0].note = midier::Note::A;
          } else {
            tracks[0].note = midier::Note::C;
          }
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
  digitalWrite(runLedPin, ledState);
}

void sendAllNotesOff(byte channel) {
  // log("sendAllNotesOff()");

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
  const midier::Degree degrees[] = { 1, 5 };

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
  const midier::Degree degrees[] = { 1, 5 };

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

int sustain = 0;
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
    if (active == '*' && previous == '*') {
      sustain++;
    }
    if (active == '*' && ((previous != '*' || (step == 0 && bars == 0)) || sustain >= 8)) {
      log("playNote()");
      sustain = 0;
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
    while (!Serial) {
      continue; // wait for serial port to connect.
  }
  Serial.println("setup()");
  randomSeed(analogRead(0));
  pinMode(ledPin, OUTPUT);
  pinMode(runLedPin, OUTPUT);
  pinMode(playButtonPin, INPUT_PULLUP);
  digitalWrite(ledPin, ledState);
  digitalWrite(runLedPin, ledState);
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
    1,
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

  Serial.print("\nInitializing SD card...");
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  // print the type of card
  Serial.print("\nCard type: ");
  switch(card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }
  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  Serial.println();
  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  if (volumesize < 8388608ul) {
    Serial.print("Volume size (bytes): ");
    Serial.println(volumesize * 512);        // SD card blocks are always 512 bytes
  }
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 2;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  
  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);
  
  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);

  // Start master clock using timer interrupt
  clk.begin(pulse, ONE_MINUTE / PPQN / bpm);

  // Transition to stopped state
  sequencer.trigger(STOP);
}

// debounce returns the state when the switch is stable
bool debounce(int pin)
{
  bool state;
  bool previousState;

  previousState = digitalRead(pin);          // store switch state
  for(int counter=0; counter < debounceDelay; counter++)
  {
      delay(1);                  // wait for 1 millisecond
      state = digitalRead(pin);  // read the pin
      if( state != previousState)
      {
         counter = 0; // reset the counter if the state changes
         previousState = state;  // and save the current state
      }
  }
  // here when the switch state has been stable longer than the debounce period
  if(state == LOW)  // LOW means pressed (because pull-ups are used)
     return true;
  else
    return false;
}

void loop() {
  bool playButtonState = debounce(playButtonPin);
  if (playButtonState)
  {
      count++; // increment count
      Serial.println(count);  // display the count on the Serial Monitor
  }

  // If the button state changed and the button was pressed
  if (lastPlayButtonState != playButtonState && playButtonState == HIGH) {
    switch (mode) {
      case RUNNING:
        sequencer.trigger(STOP);
        break;
      case PAUSED:
        sequencer.trigger(STOP);
        break;
      case STOPPED:
        sequencer.trigger(PANIC);
        break;
      case PANICKED:
        sequencer.trigger(PLAY);
        break;
    }
  }
  lastPlayButtonState = playButtonState;
}