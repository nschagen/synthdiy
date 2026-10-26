#include <MIDI.h>
#include  <SPI.h>
#include <debounce.h>

MIDI_CREATE_DEFAULT_INSTANCE(); //MIDI

const int SS2 = 8; // secondary DAC SS
const int CONFIG_BTN = A0; // button input
const int GATE_OUT = A1; // gate output pin
const int CLK_OUT = A2; // clock output pin
const int RESET_OUT = A3; // reset output pin

// Status RGB led pins
const int STATUS_LED_RED_PWM_PIN = 6;
const int STATUS_LED_GREEN_PWM_PIN = 3;
const int STATUS_LED_BLUE_PWM_PIN = 5;


int note_no = 0;//noteNo=21(C1)～60(C6) total 61

int bend_range = 0;
int bend_msb = 0;
int bend_lsb = 0;
long after_bend_pitch = 0;

byte note_on_count = 0;
unsigned long trigTimer = 0; //for gate ratch

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

const ModuleConfig defaultConfig =  { 1, CFG_NOTE_PRIO_LAST, CFG_LEGATO_OFF, CFG_RESET_STOP };
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


static Button shortPressButton(0, shortPressHandler);
static Button longPressButton(0, longPressHandler);


void reset_state() {
  note_on_count = 0;
  trigTimer = 0;  
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

  longPressButton.setPushDebounceInterval(1000); 
  
  StatusLED(0, 0, 0);
  MIDI.begin(1);
  
  SPI.begin();
  SPI.setBitOrder(MSBFIRST) ;          // bit order
  SPI.setClockDivider(SPI_CLOCK_DIV4) ;// (16MHz/4)
  SPI.setDataMode(SPI_MODE0) ;         // LOW


  digitalWrite(RESET_OUT, HIGH);
  delay(600); 
  digitalWrite(RESET_OUT, LOW); 
}



void loop() {
  unsigned long t = millis();

  // Update button states
  int btnState = digitalRead(CONFIG_BTN);
  shortPressButton.update(btnState);
  longPressButton.update(btnState);

  //--- gate retrigger when additional keys are pressed (when not in legato mode) ----------
  if (note_on_count != 0 && !currentConfig.legato) {
    if ((t - trigTimer <= 20) && (t - trigTimer > 10)) {
      digitalWrite(GATE_OUT, LOW);
    }
    if ((trigTimer > 0) && (t - trigTimer > 20)) {
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
  if (MIDI.read()) {

    int new_note_no = 0;
    
    switch (MIDI.getType()) {
    
      case midi::NoteOn:
        note_on_count ++;
        new_note_no = MIDI.getData1() - NOTE_C1;
        
        if (new_note_no < 0) {
          new_note_no = 0;
        }
        else if (new_note_no >= 61) {
          new_note_no = 60;
        }
 
        if (note_on_count == 1 || currentConfig.note_prio == CFG_NOTE_PRIO_LAST) {
          note_no = new_note_no;
          trigTimer = millis();
        } else {
          // trigTimer must only be set when a note overrides the previous one based on priority
          if (currentConfig.note_prio == CFG_NOTE_PRIO_LOW) {
            if (new_note_no < note_no) {
              note_no = new_note_no;
              trigTimer = millis();
            }
          } else {
            if (new_note_no < note_no) {
              note_no = new_note_no;
              trigTimer = millis();
            }
          }
        }

        if (!learnMode) {
          digitalWrite(GATE_OUT, HIGH); 
          OUT_CV(cv[note_no]);        
        }

        break;
      
      case midi::NoteOff:
          if (note_on_count > 0) {
            note_on_count--;
          }
          if (note_on_count == 0) {
            digitalWrite(GATE_OUT, LOW); 
          }

          if (learnMode) {
            // call configure() which starts animation and sets state!
            
            int opt = note_no % 12;
            if (opt >= 7) opt = 7;
            configure(opt, MIDI.getChannel());
            learnMode = 0;
          }
          break;
      
      case midi::ControlChange:
          if (!learnMode) {
            OUT_MOD( MIDI.getData2() << 5);
          }
          break;
      
      case midi::Clock:
          clock_count ++;
          
          if (clock_count >= clock_max) {
            clock_count = 0;
          }   
            
          if (clock_count == 0 && !learnMode) {
            digitalWrite(CLK_OUT, HIGH);
            clock_pulse_start_time = millis();
          }

          break;

      case midi::Start:
          clock_count = 0;
          digitalWrite(GATE_OUT, LOW);
          digitalWrite(CLK_OUT, LOW);

          if (currentConfig.reset_effect == CFG_RESET_START_STOP) {
            digitalWrite(RESET_OUT, HIGH);
            reset_pulse_start_time = millis();   
          }
          break;
      
      case midi::Stop:
          clock_count = 0;
          digitalWrite(GATE_OUT, LOW);
          digitalWrite(CLK_OUT, LOW);

          digitalWrite(RESET_OUT, HIGH);
          reset_pulse_start_time = millis();   
          break;
      
      case midi::PitchBend:
          if (learnMode) break;

          bend_lsb = MIDI.getData1();//LSB
          bend_msb = MIDI.getData2();//MSB
          bend_range = bend_msb; //0 to 127

          if (bend_range > 64) {
            after_bend_pitch = cv[note_no] + cv[note_no] * (bend_range - 64) * 4 / 10000;
            OUT_CV(after_bend_pitch);
          }
          else if (bend_range < 64) {
            after_bend_pitch = cv[note_no] - cv[note_no] * (64 - bend_range) * 4 / 10000;
            OUT_CV(after_bend_pitch);
          }
          break;
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
