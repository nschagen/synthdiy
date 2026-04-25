ls#include <SPI.h>

// ========= PIN CONFIG =========
const uint8_t PIN_DAC_CS = 10;

const uint8_t PIN_SR1_LATCH = 2;
const uint8_t PIN_SR1_CLOCK = 3;
const uint8_t PIN_SR1_DATA = 4;

const uint8_t PIN_SR2_LATCH = 6;
const uint8_t PIN_SR2_CLOCK = 5;
const uint8_t PIN_SR2_DATA = 7;

const uint8_t PIN_SCALE_POT_1 = A1;
const uint8_t PIN_SCALE_POT_2 = A4;
const uint8_t PIN_TRANSPOSE_POT_1 = A2;
const uint8_t PIN_TRANSPOSE_POT_2 = A5;
const uint8_t PIN_CV_IN_1 = A3;
const uint8_t PIN_CV_IN_2 = A0;

// ========= HARDWARE / CALIBRATION =========
const float DAC_REF_VOLTAGE = 5.03f;
const float ADC_REF_VOLTAGE = 5.00f;
const float INPUT_ATTN_RATIO = 0.5f;
const float OCTAVE_VOLTAGE = 1.0f;
const uint8_t NUM_OCTAVES = 5;
const uint8_t NOTES_PER_OCTAVE = 12;
const float MAX_OUTPUT_VOLTAGE = NUM_OCTAVES * OCTAVE_VOLTAGE;
const uint16_t DAC_MAX = 4095;

// ========= DISPLAY WIRING =========
// Bit order: {a,b,c,d,e,f,g,dp} in the output byte. Change these if your wiring differs.
const uint8_t SEG_BIT_A  = 0;
const uint8_t SEG_BIT_B  = 1;
const uint8_t SEG_BIT_C  = 2;
const uint8_t SEG_BIT_D  = 3;
const uint8_t SEG_BIT_E  = 4;
const uint8_t SEG_BIT_F  = 5;
const uint8_t SEG_BIT_G  = 6;
const uint8_t SEG_BIT_DP = 7;

const uint8_t SEGMENT_DIGITS[10] = {
  0b11100111, 
  0b00100001,
  0b11001011,
  0b01101011,
  0b00101101,
  0b01101110,
  0b11101110,
  0b00100011,
  0b11101111,
  0b01101111
};

// ========= SCALES =========
const uint8_t SCALE_COUNT = 10;

const uint8_t SCALE_DEGREES[SCALE_COUNT][12] = {
  {0,255,255,255,255,255,255,255,255,255,255,255}, // octave
  {0,7,255,255,255,255,255,255,255,255,255,255},  // octave + fifth
  {0,3,5,7,10,255,255,255,255,255,255,255}, // minor pentatonic
  {0,2,5,7,9,255,255,255,255,255,255,255}, // major pentatonic
  {0,2,4,5,7,9,11,255,255,255,255,255}, // major scale
  {0,2,3,5,7,9,10,255,255,255,255,255}, // dorian scale
  {0,3,5,6,7,10,255,255,255,255,255,255}, // blues scale
  {0,2,3,5,7,8,10,255,255,255,255,255}, // minor scale
  {0,1,3,5,7,8,10,255,255,255,255,255}, // phrygian scale
  {0,1,2,3,4,5,6,7,8,9,10,11} // chromatic
};
const uint8_t SCALE_LENGTHS[SCALE_COUNT] = {1,2,5,5,7,7,6,7,7,12};
const uint8_t CHROMATIC_SCALE_IDX = 9;

// ========= FILTERING / HYSTERESIS =========
const uint8_t ANALOG_SAMPLES = 1;
const uint8_t STABLE_REQUIRED = 1;
const uint8_t LOOPS_BEFORE_POT_UPDATE = 20;
const uint16_t POT_HYSTERESIS_ADC = 12;
const uint16_t INPUT_HYSTERESIS_ADC = 8;
const float TRANSPOSE_RANGE_V = 1.0f;
const float TRANSPOSE_HYSTERESIS_V = 0.03f;
const uint8_t DISPLAY_UPDATE_MS = 20;

struct StableAnalog {
  uint16_t stable = 0;
  uint16_t candidate = 0;
  uint8_t candidateCount = 0;
};

struct ChannelState {
  uint8_t scaleIndex = 0;
  uint8_t displayScaleValue = 0;
  float transposeV = 0.0f;
  float inputV = 0.0f;
  float quantizedV = 0.0f;
};

StableAnalog scalePot1, scalePot2, transposePot1, transposePot2, cvIn1, cvIn2;
ChannelState ch1, ch2;
unsigned long lastDisplayMs = 0;
unsigned long toPotUpdate = 0;

uint16_t readAnalogAverage(uint8_t pin) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < ANALOG_SAMPLES; i++) sum += analogRead(pin);
  return (sum ) / ANALOG_SAMPLES;
}

uint16_t updateStableAnalog(StableAnalog &s, uint8_t pin, uint16_t hysteresis) {
  uint16_t raw = readAnalogAverage(pin);
  return raw;
  //if (s.candidateCount == 0) {
  //  s.stable = raw;
  //  s.candidate = raw;
  //  s.candidateCount = 1;
  //  return s.stable;
  //}
  //if (abs((int)raw - (int)s.stable) <= hysteresis) {
  //  s.candidate = raw;
  //  s.candidateCount = 0;
  //  return s.stable;
  //}
  //if (abs((int)raw - (int)s.candidate) <= hysteresis) {
  //  s.candidateCount++;
  //  s.candidate = raw;
  //  if (s.candidateCount >= STABLE_REQUIRED) s.stable = raw;
  //} else {
  //  s.candidate = raw;
  //  s.candidateCount = 1;
  //}
  //return s.stable;
}

float adcToVoltage(uint16_t adc) { return (adc * ADC_REF_VOLTAGE) / 1023.0f; }
float inputToVPerOct(float adcV) { return adcV / INPUT_ATTN_RATIO; }
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

uint8_t readScaleIndex(StableAnalog &filter, uint8_t pin) {
  uint16_t adc = updateStableAnalog(filter, pin, POT_HYSTERESIS_ADC);
  uint8_t idx = (uint8_t)constrain(map(adc, 0, 1023, 0, SCALE_COUNT - 1), 0, SCALE_COUNT - 1);
  if (idx >= SCALE_COUNT) idx = SCALE_COUNT - 1;
  return idx;
}

float readTransposeV(StableAnalog &filter, uint8_t pin) {
  uint16_t adc = updateStableAnalog(filter, pin, POT_HYSTERESIS_ADC);
  float v = adcToVoltage(adc);
  // Add an extra 0.25v on both sides which acts as a deadzone
  float centered = (v / ADC_REF_VOLTAGE) * 2.5f - 1.25f;
  if (centered < 0.0f) {
    centered = clampf(centered + 0.25f, -1.0f, 0.0f);
  } else {
    centered = clampf(centered - 0.25f, 0.0f, 1.0f);
  }
  return clampf(round(centered * 12.0) / 12.0, -TRANSPOSE_RANGE_V, TRANSPOSE_RANGE_V);
}

float quantizeToScale(float vOct, uint8_t scaleIndex) {
  vOct = clampf(vOct, 0.0f, MAX_OUTPUT_VOLTAGE);
  int octave = (int)floor(vOct);
  float frac = vOct - octave;
  const uint8_t len = SCALE_LENGTHS[scaleIndex];
  const uint8_t *scale = SCALE_DEGREES[scaleIndex];
  float best = octave;
  float bestDist = 999.0f;
  for (uint8_t o = 0; o <= NUM_OCTAVES; o++) {
    for (uint8_t i = 0; i < len; i++) {
      float note = o + (scale[i] / 12.0f);
      if (note < 0.0f || note > MAX_OUTPUT_VOLTAGE) continue;
      float d = fabs(vOct - note);
      if (d < bestDist || (fabs(d - bestDist) < 0.0001f && note <= vOct)) {
        bestDist = d;
        best = note;
      }
    }
  }
  return clampf(best, 0.0f, MAX_OUTPUT_VOLTAGE);
}

uint16_t voltageToDac(float v) {
  v = clampf(v, 0.0f, MAX_OUTPUT_VOLTAGE);
  return (uint16_t)lround((v / DAC_REF_VOLTAGE) * DAC_MAX);
}

void setDac(uint16_t value, uint8_t channel) {
  value &= 0x0FFF;
  uint8_t control = 0b00110000;
  if (channel == 1) control |= 0b10000000;
  uint8_t high = control | (value >> 8);
  uint8_t low = value & 0xFF;
  digitalWrite(PIN_DAC_CS, LOW);
  SPI.transfer(high);
  SPI.transfer(low);
  digitalWrite(PIN_DAC_CS, HIGH);
}

uint8_t makeDigitByte(uint8_t digit) { return SEGMENT_DIGITS[digit % 10]; }

void write7Seg(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin, uint8_t value) {
  uint8_t b = makeDigitByte(value);
  digitalWrite(latchPin, LOW);
  for (int i = 7; i >= 0; i--) {
    digitalWrite(clockPin, LOW);
    digitalWrite(dataPin, (b >> i) & 1);
    digitalWrite(clockPin, HIGH);
  }
  digitalWrite(latchPin, HIGH);
}

void updateChannel(
  ChannelState &ch, 
  StableAnalog &scaleFilter,
  uint8_t scalePin,
  StableAnalog &transposeFilter,
  uint8_t transposePin,
  StableAnalog &cvFilter,
  uint8_t cvPin,
  uint8_t latchPin,
  uint8_t clockPin,
  uint8_t dataPin,
  uint8_t doPotUpdate
) {
  if (doPotUpdate) {
    ch.scaleIndex = readScaleIndex(scaleFilter, scalePin);
    ch.transposeV = readTransposeV(transposeFilter, transposePin);
  }
  uint16_t adc = updateStableAnalog(cvFilter, cvPin, INPUT_HYSTERESIS_ADC);
  ch.inputV = adcToVoltage(adc);
  ch.quantizedV = quantizeToScale(ch.inputV, ch.scaleIndex);

  float transposed = clampf(ch.quantizedV + ch.transposeV, 0.0f, MAX_OUTPUT_VOLTAGE);
  setDac(voltageToDac(transposed), (cvPin == PIN_CV_IN_1) ? 1 : 0);
  write7Seg(latchPin, clockPin, dataPin, ch.scaleIndex);
}

void setup() {
  pinMode(PIN_DAC_CS, OUTPUT);
  pinMode(PIN_SR1_LATCH, OUTPUT);
  pinMode(PIN_SR1_CLOCK, OUTPUT);
  pinMode(PIN_SR1_DATA, OUTPUT);
  pinMode(PIN_SR2_LATCH, OUTPUT);
  pinMode(PIN_SR2_CLOCK, OUTPUT);
  pinMode(PIN_SR2_DATA, OUTPUT);
  digitalWrite(PIN_DAC_CS, HIGH);
  digitalWrite(PIN_SR1_LATCH, HIGH);
  digitalWrite(PIN_SR2_LATCH, HIGH);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
}

void loop() {
  uint8_t doPotUpdate = 0;
  toPotUpdate++;
  if (toPotUpdate > LOOPS_BEFORE_POT_UPDATE) {
    toPotUpdate = 0;
    doPotUpdate = 1;
  }
  updateChannel(ch1, scalePot1, PIN_SCALE_POT_1, transposePot1, PIN_TRANSPOSE_POT_1, cvIn1, PIN_CV_IN_1, PIN_SR1_LATCH, PIN_SR1_CLOCK, PIN_SR1_DATA, doPotUpdate);
  updateChannel(ch2, scalePot2, PIN_SCALE_POT_2, transposePot2, PIN_TRANSPOSE_POT_2, cvIn2, PIN_CV_IN_2, PIN_SR2_LATCH, PIN_SR2_CLOCK, PIN_SR2_DATA, doPotUpdate);
}
