#include <MIDI.h> // https://github.com/FortySevenEffects/arduino_midi_library
#include  <SPI.h>
#include <debounce.h> // https://github.com/kimballa/button-debounce
#include <SoftwareSerial.h>

MIDI_CREATE_DEFAULT_INSTANCE(); //MIDI

const int CONFIG_BTN = A0; // button input
const int GATE_OUT = A1; // gate output pin
const int CLK_OUT = A2; // clock output pin
const int RESET_OUT = A3; // reset output pin

const int DEBUG_BTN = 2; // dump state button pin

// Status RGB led pins
const int STATUS_LED_RED_PWM_PIN = 6;
const int STATUS_LED_GREEN_PWM_PIN = 3;
const int STATUS_LED_BLUE_PWM_PIN = 5;

int bend_range = 0;
int bend_msb = 0;
int bend_lsb = 0;

const uint8_t NUM_MIDI_NOTES = 61;
const uint8_t MAX_HELD_NOTES = 10;

int note_no = 0;//noteNo=21(C1)～60(C6) total 61
bool noteHeld[NUM_MIDI_NOTES];       // 0 = 21 = C1
uint8_t heldOrder[MAX_HELD_NOTES];   // chronological list of held note numbers
uint8_t heldCount = 0;
unsigned long noteTrigTimer = 0; //for gate ratch

long after_bend_pitch = 0;

const uint8_t NOTE_A0 = 21;
const uint8_t NOTE_C1 = 24;

byte clock_count = 0;
byte clock_max = 1; // implements clock division. Values taken from CLOCK_DIVS
long clock_pulse_start_time = 0;
long reset_pulse_start_time = 0;

// V/OCT LSB for DAC
const long cv[61] = {
 0,  68, 137,  205,  273,  341,  410,  478,  546,  614,  683,  751,
 819,  887,  956,  1024, 1092, 1161, 1229, 1297, 1365, 1434, 1502, 1570,
 1638, 1707, 1775, 1843, 1911, 1980, 2048, 2116, 2185, 2253, 2321, 2389,
 2458, 2526, 2594, 2662, 2731, 2799, 2867, 2935, 3004, 3072, 3140, 3209,
 3277, 3345, 3413, 3482, 3550, 3618, 3686, 3755, 3823, 3891, 3959, 4028, 4095
};

const long SEMITONE_CV = 68;

// A clock pulse takes 5ms
const uint8_t CLOCK_PULSE_DURATION = 5;

// 0 = inactive, 1 = waiting, 2-14 = learned !
// learned indicates that a key was released (octave has 12 keys) and that a specific setting was learned
// this learned state will remain while the status LED indicates feedback. 
// After 1 second, the device will reset to normal operation
int learnMode = 0; 

// indicates a particular LED animation that is playing
long ledAnimationStart = 0;
int ledAnimation = 0;

// animation constants
const int LEARN_MODE_ANIM = 1;
const int LEARNED_MIDI_CHAN = 2;
const int LEARNED_NOTE_PRIO_LOW = 3;
const int LEARNED_NOTE_PRIO_LAST = 4;
const int LEARNED_NOTE_PRIO_HIGH = 5;
const int LEARNED_LEGATO_OFF = 6;
const int LEARNED_LEGATO_ON = 7;
const int LEARNED_RESET_IS_STOP = 8;
const int LEARNED_RESET_IS_START_STOP = 9;
const int SET_CLOCK_DIV_ANIM = 10;

// configuration options (value = note in octave, 0 = C)
const int OPT_MIDI_CHAN = 0;
const int OPT_NOTE_PRIO_LOW = 1;
const int OPT_NOTE_PRIO_LAST = 2;
const int OPT_NOTE_PRIO_HIGH = 3;
const int OPT_TOGGLE_LEGATO = 4;
const int OPT_TOGGLE_RESET = 5;

// Values to use inside ModuleConfig
const int CFG_NOTE_PRIO_LOW = 0;
const int CFG_NOTE_PRIO_LAST = 1;
const int CFG_NOTE_PRIO_HIGH = 2;
const int CFG_LEGATO_ON = 1;
const int CFG_LEGATO_OFF = 0;
const int CFG_RESET_STOP = 0;
const int CFG_RESET_START_STOP = 1;

// clock divisions and selected division (ranges from 24ppq to 1 note)
const long CLOCK_DIVS[7] = {1, 3, 6, 12, 24, 48, 96};

struct ModuleConfig {
  uint8_t clock_div; // index into CLOCK_DIVS
  uint8_t chan;
  uint8_t note_prio;
  uint8_t legato;
  uint8_t reset_effect;
};

const ModuleConfig defaultConfig =  { 1, CFG_NOTE_PRIO_LOW, CFG_LEGATO_OFF, CFG_RESET_STOP };
ModuleConfig currentConfig = defaultConfig;

// an animation switches between color1 and 2 after 'blink_time' miliseconds and lasts for 'duration'
struct LEDAnimation {
  uint8_t r1, g1, b1;
  uint8_t r2, g2, b2;
  unsigned long blink_time;
  unsigned long duration;    // 0 = indefinite
};

const LEDAnimation animations[] = {
  // name                     r1   g1   b1   r2   g2   b2 blink_time duration
  /* No Amination        */ {   0,   0,   0,   0,   0,   0,     0,  0 },
  /* Learn mode          */ { 255, 255, 255,   0,   0,   0,   250,  0 },
  /* Set MIDI chan       */ {   0,   0,   255, 0,   0,   0,   200,  1000 },
  /* Note prio = low     */ { 255,  10,   0,   0,   0,   0,  1000,  1000 },
  /* Note prio = last    */ { 255,  10,   0,   0,   0,   0,   200,  1000 },
  /* Note prio = high    */ { 255,  10,   0,   0,   0,   0,    50,  1000 },
  /* Legato = Off        */ { 255,   0, 255,   0,   0,   0,   100,  1000 },
  /* Legato = On         */ { 255,   0, 255,   0,   0,   0,  1000,  1000 },  
  /* Reset = Stop        */ { 255,  50,   0,   0,   0,   0,  1000,  1000 },
  /* Reset = Start + Stop*/ { 255,  50,   0,   0,   0,   0,   200,  1000 },
  /* Set clock division  */ { 255, 255, 255,   0,   0,   0,   50,     50 },
  /* Factory reset       */ { 255,   0,   0, 255,  60,   0,   100,  1000 },
};


static void shortPressHandler(uint8_t btnId, uint8_t btnState) {
  if (btnState == BTN_OPEN) {
    // cycle clock division
    currentConfig.clock_div += 1;
    if (currentConfig.clock_div >= 7) currentConfig.clock_div = 0;
    clock_max = CLOCK_DIVS[currentConfig.clock_div];
  
    // set animation for clock division change
    ledAnimation = SET_CLOCK_DIV_ANIM;
    ledAnimationStart = millis();    
  }
}

// long press enters learn mode
static void longPressHandler(uint8_t btnId, uint8_t btnState) {
  if (btnState == BTN_OPEN) {
    // enter learn mode and start its animation
    learnMode = 1;
    ledAnimation = LEARN_MODE_ANIM;
    ledAnimationStart = millis();

    MIDI.setInputChannel(MIDI_CHANNEL_OMNI);
  }
}


// sets or toggles a configuration option.
static void configure(int option, int channel) {
  // We always set the channel to wherever the configuration input came from
  MIDI.setInputChannel(channel);
  
  unsigned long t = millis();
  switch (option) {
    case OPT_MIDI_CHAN:
      currentConfig.chan = channel;
      ledAnimation = LEARNED_MIDI_CHAN;
      ledAnimationStart = t;
      break;
    case OPT_NOTE_PRIO_LOW:
      currentConfig.note_prio = CFG_NOTE_PRIO_LOW;
      ledAnimation = LEARNED_NOTE_PRIO_LOW;
      ledAnimationStart = t;
      break;
    case OPT_NOTE_PRIO_LAST:
      currentConfig.note_prio = CFG_NOTE_PRIO_LAST;
      ledAnimation = LEARNED_NOTE_PRIO_LAST;
      ledAnimationStart = t;
      break;
    case OPT_NOTE_PRIO_HIGH:
      currentConfig.note_prio = CFG_NOTE_PRIO_HIGH;
      ledAnimation = LEARNED_NOTE_PRIO_HIGH;
      ledAnimationStart = t;
      break;
    case OPT_TOGGLE_LEGATO:
      currentConfig.legato = !currentConfig.legato;
      ledAnimation = LEARNED_LEGATO_OFF + currentConfig.legato;
      ledAnimationStart = t;
      break;
    case OPT_TOGGLE_RESET:
      currentConfig.reset_effect = !currentConfig.reset_effect;
      ledAnimation = LEARNED_RESET_IS_STOP + currentConfig.reset_effect;
      ledAnimationStart = t;
      break;
    default:
      // disable animation.
      ledAnimation = 0;
      break;
  }
}


// returns true when active note has changed or when the first note was pressed, false otherwise
uint8_t handleNoteOn(uint8_t note, uint8_t velocity) {
  if (velocity == 0) {
    return handleNoteOff(note, 0);
  }

  int oldHeldCount = heldCount;

  if (!noteHeld[note]) {
    if (heldCount < MAX_HELD_NOTES) {
      heldOrder[heldCount++] = note;
    } else {
      // Optional: steal the oldest note
      removeHeldNote(heldOrder[0]);
      heldOrder[heldCount++] = note;
    }
    noteHeld[note] = true;
  }

  return updateActiveNote() || (oldHeldCount == 0);
}

// returns true when active note has changed, false otherwise
uint8_t handleNoteOff(uint8_t note, uint8_t velocity) {
  if (noteHeld[note]) {
    removeHeldNote(note);
    return updateActiveNote();
  }
  return false;
}

void removeHeldNote(uint8_t note) {
  noteHeld[note] = false;

  for (uint8_t i = 0; i < heldCount; i++) {
    if (heldOrder[i] == note) {
      for (uint8_t j = i; j < heldCount - 1; j++) {
        heldOrder[j] = heldOrder[j + 1];
      }
      heldCount--;
      break;
    }
  }
}

// updates note_no according to notes held and priority 
// returns 1 when the active note changed, 0 otherwise
uint8_t updateActiveNote() {
  if (heldCount == 0) return 0;

  int old_note_no = note_no;

  switch (currentConfig.note_prio) {
    case CFG_NOTE_PRIO_LOW: {
      for (uint8_t i = 0; i < NUM_MIDI_NOTES; i++) {
        if (noteHeld[i]) {
          note_no = i;
          break;
        }
      }
      break;
    }

    case CFG_NOTE_PRIO_HIGH: {
      for (int8_t i = NUM_MIDI_NOTES -1; i >= 0; i--) {
        if (noteHeld[i]) {
          note_no = i;
          break;
        }
      }
      break;
    }

    case CFG_NOTE_PRIO_LAST:
    default:
      note_no = heldOrder[heldCount - 1];
  }

  // did the current note change?
  return (note_no != old_note_no);
}

void debugButtonHandler(uint8_t btnId, uint8_t btnState) {
  if (btnState == BTN_OPEN) {
    StatusLED(0, 0, 255);
    delay(100);
    StatusLED(0, 0, 0);
    Serial.print("note ");
    Serial.print(note_no);
    Serial.println("");
    Serial.print("held count ");
    Serial.print(heldCount);
    Serial.println("");
    Serial.print("noteTrigTimer ");
    Serial.print(noteTrigTimer);
    Serial.println("");
    Serial.print("learnMode ");
    Serial.print(learnMode);
    Serial.println("");
  }
}


static Button shortPressButton(0, shortPressHandler);
static Button longPressButton(0, longPressHandler);

static Button debugButton(1, debugButtonHandler);

int read_note() {
  int note = MIDI.getData1() - NOTE_C1;
  if (note < 0) note = 0;
  if (note >= 61) note = 60;   
  return note;
}

void reset_state() {
  heldCount = 0;
  noteTrigTimer = 0;  

  for (int8_t i = NUM_MIDI_NOTES -1; i >= 0; i--) {
    noteHeld[i] = 0;
  }
  for (int8_t i = MAX_HELD_NOTES -1; i >= 0; i--) {
    heldOrder[i] = 0;
  }
  learnMode = 0;

  // reset clock / pulse bookkeeping
  clock_count = 0;
  clock_max = 1;
  clock_pulse_start_time = 0;
  reset_pulse_start_time = 0;
  
  // reset LED animation
  ledAnimation = 0;
  ledAnimationStart = 0;
  StatusLED(0, 0, 0);
}

void setup() {
  pinMode(SS, OUTPUT) ; // SELECT DAC
  pinMode(CLK_OUT, OUTPUT);
  pinMode(GATE_OUT, OUTPUT);
  pinMode(RESET_OUT, OUTPUT);
  
  // RGB status LED PWM outputs
  pinMode(STATUS_LED_RED_PWM_PIN, OUTPUT);
  pinMode(STATUS_LED_GREEN_PWM_PIN, OUTPUT);
  pinMode(STATUS_LED_BLUE_PWM_PIN, OUTPUT);
  
  pinMode(CONFIG_BTN, INPUT_PULLUP);
  pinMode(DEBUG_BTN, INPUT_PULLUP);

  longPressButton.setPushDebounceInterval(1000); 
  
  StatusLED(255, 0, 255);
  delay(100);
  StatusLED(0, 0, 0);  
  MIDI.begin(1);
  MIDI.turnThruOff();
  
  SPI.begin();
  SPI.setBitOrder(MSBFIRST) ;          // bit order
  SPI.setClockDivider(SPI_CLOCK_DIV4) ;// (16MHz/4)
  SPI.setDataMode(SPI_MODE0) ;         // LOW

  Serial.println("started\n");
}

void loop() {
  unsigned long t = millis();

  // Update button states
  int btnState = digitalRead(CONFIG_BTN);
  shortPressButton.update(btnState);
  longPressButton.update(btnState);

  int debugBtnState = digitalRead(DEBUG_BTN);
  debugButton.update(debugBtnState);

  //--- gate retrigger when additional keys are pressed (when not in legato mode) ----------
  if (heldCount != 0 && !currentConfig.legato) {
    if ((t - noteTrigTimer <= 20) && (t - noteTrigTimer > 10)) {
      digitalWrite(GATE_OUT, LOW);
    }
    if ((noteTrigTimer > 0) && (t - noteTrigTimer > 20)) {
      digitalWrite(GATE_OUT, HIGH);
    }
  }

  // ensure clock pulse stops in time
  if (clock_pulse_start_time > 0 && clock_pulse_start_time < t - CLOCK_PULSE_DURATION) {
    clock_pulse_start_time = t;
    digitalWrite(CLK_OUT, LOW);
  }

  // ensure clock pulse stops in time
  if (reset_pulse_start_time > 0 && reset_pulse_start_time < t - CLOCK_PULSE_DURATION) {
    reset_pulse_start_time = t;
    digitalWrite(RESET_OUT, LOW);
  }

  if (t > 0 && ledAnimation > 0 && ledAnimationStart > 0) {
    // A LED animation is playing
    unsigned long duration = animations[ledAnimation].duration;
    
    long progress = (t - ledAnimationStart);
    if (progress > duration && duration > 0) {
      // animation has ended
      ledAnimation = 0;
      ledAnimationStart = 0;
      StatusLED(0, 0, 0);
    } else {
      if ((progress / animations[ledAnimation].blink_time) % 2 == 0) {
        StatusLED(
          animations[ledAnimation].r1, 
          animations[ledAnimation].g1, 
          animations[ledAnimation].b1
        );    
      } else {
        StatusLED(
          animations[ledAnimation].r2, 
          animations[ledAnimation].g2, 
          animations[ledAnimation].b2
        );        
      }
    }
  }

  // modes:
  // regular operation
  //    learn LED: shows clock division color (pulses according to clock)
  //    white pulse when RESET outputs
  //
  // short press: change clock divider (raw, 1/32, 1/16, 1/8, 1/4, 1/2, 1/1)
  //    learn LED changes color to new division
  // 
  // long press (1 seconds+ and release!), no clock -> learn mode (OMNI)
  //    waits for single event and exits learn mode
  //    LED is blinking white
  //    short button press exits learn mode as well
  // note on (each setting has color), gate LED indicates if setting is on or off (if applicable)
  //  C (on any channel) -> set channel
  //  A, A# or B (current channel) -> set note prio
  //  E -> toggle legato (on/off). when on, we briefly pull gate low when new note on
  //  F -> toggle reset (start or stop+start)
  //  G -> cycle mod source (velocity, aftertouch)  
  //
  // when selected a setting, the learn LED blinks that color for 500ms

  // reset settings: button held for over 8 sec. After 4 seconds it will blink RED/YELLOW

  // MIDI IN / MIDI TRU
  // LEARN BTN / LED
  //
  // GATE / LED
  // CV   / MOD
  // CLK  / RST

  //-----------------------------midi operation----------------------------
  int opt = 0;
  int new_note_no = 0;
  int released_note_no = 0;
  int oldHeldCount = 0;
  
  if (MIDI.read()) {
    
    switch (MIDI.getType()) {
    
      case midi::NoteOn: {
        new_note_no = read_note();
        oldHeldCount = heldCount;

        if (handleNoteOn(new_note_no, MIDI.getData2())) {
          // Note pressed, released or active note has changed!

          // handleNoteOn may process noteOn events with velocity=0, which are handled as noteOff
          // therefore, we compare oldHeldCount to heldCount to 
          // distinguish NoteOn from NoteOff and also handle a heldCount==0 case.
          
          OUT_CV(cv[note_no]);
                        
          if (heldCount > oldHeldCount && !learnMode) {
            // New note pressed    
            noteTrigTimer = millis();
            digitalWrite(GATE_OUT, HIGH);   
          } 

          if (heldCount == 0) {
            // last note is released. 
            digitalWrite(GATE_OUT, LOW); 
  
            if (learnMode) {
              // call configure() which starts animation and sets state!
              opt = note_no % 12;
              if (opt >= 7) opt = 7;
              configure(opt, MIDI.getChannel());
              learnMode = 0;
            }   
          }
        }
        break;
      }
      case midi::NoteOff: {
          released_note_no = read_note();

          if (handleNoteOff(released_note_no, MIDI.getData2())) {
            // note released but changing to other
            OUT_CV(cv[note_no]);              
          }

          if (heldCount == 0) {
            // last note is released. 
            digitalWrite(GATE_OUT, LOW); 

            if (learnMode) {
              // call configure() which starts animation and sets state!
              
              opt = note_no % 12;
              if (opt >= 7) opt = 7;
              configure(opt, MIDI.getChannel());
              learnMode = 0;
            }   
          }
          break;
      }
      case midi::ControlChange: {
          if (!learnMode) {
            OUT_MOD( MIDI.getData2() << 5);
          }
          break;
      }
      case midi::Clock: {
          clock_count ++;
          
          if (clock_count >= clock_max) {
            clock_count = 0;
          }   
            
          if (clock_count == 0 && !learnMode) {
            digitalWrite(CLK_OUT, HIGH);
            clock_pulse_start_time = millis();
          }

          break;
      }
      case midi::Start: {
          clock_count = 0;
          digitalWrite(GATE_OUT, LOW);
          digitalWrite(CLK_OUT, LOW);

          if (currentConfig.reset_effect == CFG_RESET_START_STOP) {
            digitalWrite(RESET_OUT, HIGH);
            reset_pulse_start_time = millis();   
          }
          break;
      }
      case midi::Stop: {
          clock_count = 0;
          digitalWrite(GATE_OUT, LOW);
          digitalWrite(CLK_OUT, LOW);

          digitalWrite(RESET_OUT, HIGH);
          reset_pulse_start_time = millis();   
          break;
      }
      case midi::PitchBend: {
          if (learnMode) break;
  
          int lsb = MIDI.getData1();          // 0..127
          int msb = MIDI.getData2();          // 0..127
          int bend14 = (msb << 7) | lsb;      // 0..16383
  
          // Signed bend: -8192..+8191
          int bend = bend14 - 8192;
  
          // Example: +/- 2 semitones over full wheel travel.
          // semitone_cv must be the CV increment for one semitone.
          float bend_cv = (bend / 8192.0f) * (2.0f * SEMITONE_CV);
  
          float after_bend_pitch = cv[note_no] + bend_cv;
          OUT_CV(after_bend_pitch);
          break;
      }
    }
  }
}


void OUT_CV(int cv) {
 digitalWrite(SS, LOW) ;
 SPI.transfer((cv >> 8) | 0x30) ;   // H0x30=OUTA/1x
 SPI.transfer(cv & 0xff) ;
 digitalWrite(SS, HIGH) ;
}

void OUT_MOD(int mod) {
 digitalWrite(SS, LOW) ;
 SPI.transfer((mod >> 8) | 0xB0) ;   // H0xB0=OUTB/1x
 SPI.transfer(mod & 0xff) ;
 digitalWrite(SS, HIGH) ;
}

void StatusLED(int redValue, int greenValue, int blueValue) {
  analogWrite(STATUS_LED_RED_PWM_PIN, redValue);
  analogWrite(STATUS_LED_GREEN_PWM_PIN, greenValue);
  analogWrite(STATUS_LED_BLUE_PWM_PIN, blueValue);
}
