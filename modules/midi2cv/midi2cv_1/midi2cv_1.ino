#include <MIDI.h>
#include  <SPI.h>
MIDI_CREATE_DEFAULT_INSTANCE(); //MIDI

const int SS2 = 8; // secondary DAC SS
const int CLK_OUT = 4; // clock output pin
const int GATE_OUT = 2; // gate1 output pin
const int GATE2_OUT = 3; // gate2 output pin

// Status RGB led pins
const int STATUS_LED_RED_PWM_PIN = 5;
const int STATUS_LED_GREEN_PWM_PIN = 6;
const int STATUS_LED_BLUE_PWM_PIN = 9; 

// Inputs
const int SINGLE_DUAL_VOICE_SWITCH = 14;
const int CONFIG_BTN = 7;

int note_no = 0;//noteNo=21(A0)～60(A5) total 61


const int BTN_NONE = 100;
const int BTN_PRESSED = 0;
const int BTN_RELEASED = 1;

struct buttonDebounceData {
  int buttonPin;
  int debounceTime;
  unsigned long lastDebounceTime;
  int state;
  int lastReading;
};

buttonDebounceData config_btn = {CONFIG_BTN, 50, 0, -1, -1};

int config_state = 0; 

int bend_range = 0;
int bend_msb = 0;
int bend_lsb = 0;
long after_bend_pitch = 0;

byte note_on_count = 0;
unsigned long trigTimer = 0;//for gate ratch

byte clock_count = 0;
byte clock_max = 24;//clock_max change by knob setting
byte clock_on_time = 0;
int clock_rate = 0;//knob CVin

// V/OCT LSB for DAC
const long cv[61] = {
 0,  68, 137,  205,  273,  341,  410,  478,  546,  614,  683,  751,
 819,  887,  956,  1024, 1092, 1161, 1229, 1297, 1365, 1434, 1502, 1570,
 1638, 1707, 1775, 1843, 1911, 1980, 2048, 2116, 2185, 2253, 2321, 2389,
 2458, 2526, 2594, 2662, 2731, 2799, 2867, 2935, 3004, 3072, 3140, 3209,
 3277, 3345, 3413, 3482, 3550, 3618, 3686, 3755, 3823, 3891, 3959, 4028, 4095
};

void setup() {
 pinMode(SS, OUTPUT) ; // SELECT DAC1
 pinMode(SS2, OUTPUT) ; // SELECT DAC2
 pinMode(CLK_OUT, OUTPUT) ; //CLK_OUT
 pinMode(GATE_OUT, OUTPUT) ; //GATE_OUT (1)
 pinMode(GATE2_OUT, OUTPUT) ; //GATE_OUT (2)

 // RGB status LED PWM outputs
 // blue = midi message received
 // red = configure voice 1 (cv/gate)
 // yellow = configure voice 2 (cv/gate)
 // green = configure mod 1
 // pink = configure mod 2
 pinMode(STATUS_LED_RED_PWM_PIN, OUTPUT);
 pinMode(STATUS_LED_GREEN_PWM_PIN, OUTPUT);
 pinMode(STATUS_LED_BLUE_PWM_PIN, OUTPUT);

 pinMode(CONFIG_BTN, INPUT_PULLUP); // configuration push button
 pinMode(SINGLE_DUAL_VOICE_SWITCH, INPUT_PULLUP); // select single or dual voice mode

 config_state = 0;
 StatusLED(0, 0, 0);
 MIDI.begin(1);          // MIDI CH1をlisten

 SPI.begin();
 SPI.setBitOrder(MSBFIRST) ;          // bit order
 SPI.setClockDivider(SPI_CLOCK_DIV4) ;// (16MHz/4)
 SPI.setDataMode(SPI_MODE0) ;         // LOW
 delay(50);

  StatusLED(255, 0, 0);
  delay(100);
  StatusLED(0, 255, 0);
  delay(100);
  StatusLED(0, 0, 255);
  delay(100);
  StatusLED(0, 0, 0);
  delay(100);
  StatusLED(0, 0, 255);
  delay(100);
  StatusLED(0, 0, 0);
  delay(100);
  StatusLED(0, 0, 255);
  delay(100);
  StatusLED(0, 0, 0);
  delay(100);
  StatusLED(0, 0, 255);
  delay(100);
  StatusLED(0, 0, 0);
}

// handles debouncing a button
bool readButtonState(buttonDebounceData btn) {
  // read button state
  int reading = digitalRead(btn.buttonPin);

  // update time since last change
  if (reading != btn.lastReading) {
    btn.lastDebounceTime = millis();
  }

  btn.lastReading = reading;

  if ((millis() - btn.lastDebounceTime) > btn.debounceTime) {

    StatusLED(255, 255, 0);
    delay(30);
    StatusLED(0, 0, 0); 
    // reading is stable for some time
    if (reading != btn.state) {
      btn.state = reading;

      //if (btn.state == 0) {
      //  StatusLED(0, 255, 255);
      //} else {
      //  StatusLED(255, 255, 0);
      //}
      //delay(300);
      //StatusLED(0, 0, 0); 

      //if (btn.state == 0) {
      return true;
      //}
    }    
  }

  return false;
}

void loop() {
  if (readButtonState(config_btn)) {
    StatusLED(255, 0, 0);
    delay(100);
    StatusLED(0, 0, 0);
  }
  
    // config
    // CVGATE1_CHANNEL
    // CVGATE2_CHANNEL
    // MOD1_CHANNEL / MOD1_CONTROLLER
    // MOD2_CHANNEL / MOD2_CONTROLLER

    // detect channel switch change
      // reset both channel states (gate off)


    // read MIDI message
        // switch
            // note on
            // note off
            // control change
            // clock
            // stop
            // pitchbend
        // store in variables

    // in config mode ?
      // switch
        // cvgate1
          // if noteOn:
            // update cvgate1_channel to channel of message
            // reset channel state
        // cvgate2
           // if noteOn:
            // update cvgate2_channel to channel of message
            // reset channel state
        // mod1
          // if controlChange
            // update mod1 channel
            // update mod1 controller
            // reset mod1 state
        // mod2
      // if configuring
      // process config update & reset


    // if duplicate_first_channel?
      // if msg.chan == cvgate1_channel:
          // update channel 1 (lowest priority)
          // update channel 2 (highest priority)
    // else:
      // if msg.chan == cvgate1_channel:
        // update channel 1 (last priority)      
      // if msg.chan == cvgate2_channel:
        // update channel 2 (last priority)


 //-----------------------------clock_rate setting----------------------------
 //clock_rate = analogRead(1); //read knob voltage
 
 //if (clock_rate < 256) {
 //  clock_max = 24;//slow
 //}
 //else if (clock_rate < 512 && clock_rate >= 256) {
 //  clock_max = 12;
 //}
 //else if (clock_rate < 768 && clock_rate >= 512) {
 //  clock_max = 6;
 //}
 //else if (clock_rate >= 768) {
 //  clock_max = 3;//fast
 //}
 clock_max = 6;

  
 //-----------------------------gate ratch----------------------------
 if (note_on_count != 0) {
   if ((millis() - trigTimer <= 20) && (millis() - trigTimer > 10)) {
     digitalWrite(GATE_OUT, LOW);
     //StatusLED(0, 0, 0);
   }
   if ((trigTimer > 0) && (millis() - trigTimer > 20)) {
     digitalWrite(GATE_OUT, HIGH);
     //StatusLED(255, 128, 0);
   }
 }

 //-----------------------------midi operation----------------------------
 if (MIDI.read()) {         
   MIDI.setInputChannel(1);
 
   switch (MIDI.getType()) {

     case midi::NoteOn:
       note_on_count ++;
       trigTimer = millis();
       note_no = MIDI.getData1() - 21 ;//note number

       if (note_no < 0) {
         note_no = 0;
       }
       else if (note_no >= 61) {
         note_no = 60;
       }

       digitalWrite(GATE_OUT, HIGH); //GateHIGH
       //StatusLED(255, 128, 0);
       OUT_CV(cv[note_no]);//V/OCT LSB for DAC
       OUT_CV2(cv[note_no]);
       break;
 

     case midi::NoteOff://NoteOff
       note_on_count --;
       if (note_on_count == 0) {
         digitalWrite(GATE_OUT, LOW); //GateLOW
         //StatusLED(0, 0, 0);
       }
       break;


     case midi::ControlChange:
       OUT_MOD( MIDI.getData2() << 5); //0-4095
       OUT_MOD2( MIDI.getData2() << 5);
       break;


     case midi::Clock:
       clock_count ++;

       if (clock_count >= clock_max) {
         clock_count = 0;
       }

       if (clock_count == 1) {
         digitalWrite(CLK_OUT, HIGH);
       }
       else if (clock_count != 1) {
         digitalWrite(CLK_OUT, LOW);
       }
       break;


     case midi::Stop:
       clock_count = 0;
       digitalWrite(GATE_OUT, LOW); //GateLOW
       //StatusLED(0, 0, 0);
       break;


     case midi::PitchBend:
       bend_lsb = MIDI.getData1();//LSB
       bend_msb = MIDI.getData2();//MSB
       bend_range = bend_msb; //0 to 127

       if (bend_range > 64) {
         after_bend_pitch = cv[note_no] + cv[note_no] * (bend_range - 64) * 4 / 10000;
         OUT_CV(after_bend_pitch);
         OUT_CV2(after_bend_pitch);
       }

       else if (bend_range < 64) {
         after_bend_pitch = cv[note_no] - cv[note_no] * (64 - bend_range) * 4 / 10000;
         OUT_CV(after_bend_pitch);
         OUT_CV2(after_bend_pitch);
       }
       break;


   }
 }
}


//DAC_CV output
void OUT_CV(int cv) {
 digitalWrite(SS, LOW) ;
 SPI.transfer((cv >> 8) | 0x30) ;   // H0x30=OUTA/1x
 SPI.transfer(cv & 0xff) ;
 digitalWrite(SS, HIGH) ;
}

//DAC_MOD output
void OUT_MOD(int mod) {
 digitalWrite(SS, LOW) ;
 SPI.transfer((mod >> 8) | 0xB0) ;   // H0xB0=OUTB/1x
 SPI.transfer(mod & 0xff) ;
 digitalWrite(SS, HIGH) ;
}

void OUT_CV2(int cv) {
 digitalWrite(SS2, LOW) ;
 SPI.transfer((cv >> 8) | 0x30) ;
 SPI.transfer(cv & 0xff) ;
 digitalWrite(SS2, HIGH) ;
}

void OUT_MOD2(int mod) {
 digitalWrite(SS2, LOW) ;
 SPI.transfer((mod >> 8) | 0xB0) ;
 SPI.transfer(mod & 0xff) ;
 digitalWrite(SS2, HIGH) ;
}

void StatusLED(int redValue, int greenValue, int blueValue) {
  analogWrite(STATUS_LED_RED_PWM_PIN, redValue);
  analogWrite(STATUS_LED_GREEN_PWM_PIN, greenValue);
  analogWrite(STATUS_LED_BLUE_PWM_PIN, blueValue);
}
