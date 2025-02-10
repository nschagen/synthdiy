#include <SPI.h>

const int DAC_CS_PIN = 10;

// analog inputs
const int SHAPE_PIN = 0;
const int RATE_PIN = 1;
const int CV1_PIN = 2;
const int CV2_PIN = 3;
const int CV3_PIN = 4;
const int SW_PIN = 5;

// pwm outputs
const int LED_R_PIN = 6;
const int LED_G_PIN = 5;
const int LED_B_PIN = 3;

// Function to set the DAC, Accepts the Value to be sent and the channel of the DAC to be used.
void setDac(int value, int channel) {
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  byte dacRegister = 0b00010000;                        // Sets default DAC registers B00110000, 1st bit choses DAC, A=0 B=1, 2nd Bit bypasses input Buffer, 3rd bit sets output gain to 1x, 4th bit controls active low shutdown. LSB are insignifigant here.
  int dacSecondaryByteMask = 0b0000000011111111;        // Isolates the last 8 bits of the 12 bit value, B0000000011111111.
  byte dacPrimaryByte = (value >> 8) | dacRegister;     //Value is a maximum 12 Bit value, it is shifted to the right by 8 bytes to get the first 4 MSB out of the value for entry into th Primary Byte, then ORed with the dacRegister
  byte dacSecondaryByte = value & dacSecondaryByteMask; // compares the 12 bit value to isolate the 8 LSB and reduce it to a single byte.
  // Sets the MSB in the primaryByte to determine the DAC to be set, DAC A=0, DAC B=1
  switch (channel) {
    case 0:
      dacPrimaryByte &= ~(1 << 7);
      break;
    case 1:
      dacPrimaryByte |= (1 << 7);
  }
  //noInterrupts(); // disable interupts to prepare to send data to the DAC
  digitalWrite(DAC_CS_PIN, LOW); // take the Chip Select pin low to select the DAC:
  SPI.transfer(dacPrimaryByte); //  send in the Primary Byte:
  SPI.transfer(dacSecondaryByte);// send in the Secondary Byte
  digitalWrite(DAC_CS_PIN, HIGH); // take the Chip Select pin high to de-select the DAC:
  //interrupts(); // Enable interupts

  SPI.endTransaction();
}

const unsigned int  steps = 512;

unsigned int  x;
unsigned int y;
unsigned int sines_of_steps[steps];

void setup() {
   int i;

   for (i = 0; i < steps; i++)
   {
      sines_of_steps[i] = round((sin(TWO_PI / steps * i) + 1) * 2047.5);
   }

  pinMode (DAC_CS_PIN, OUTPUT);
  digitalWrite(DAC_CS_PIN, HIGH);

  x = 0;
  y = 0;

  SPI.begin();
}

void loop() {
    setDac(sines_of_steps[x], 0);
    setDac(sines_of_steps[steps - x - 1], 1);

    int switch_val = analogRead(SW_PIN);
    if (switch_val < 300) {
      analogWrite(LED_R_PIN, y);
      analogWrite(LED_G_PIN, 0);
      analogWrite(LED_B_PIN, 0);
    } else if (switch_val < 1000) {
      analogWrite(LED_R_PIN, 0);
      analogWrite(LED_G_PIN, y);
      analogWrite(LED_B_PIN, 0);
    } else {
      analogWrite(LED_R_PIN, 0);
      analogWrite(LED_G_PIN, 0);
      analogWrite(LED_B_PIN, y);
    }

    x = x + 1;
    y = y + 1;
    if (x == steps) {
      x = 0;
    }
    if (y >= 255) {
      y = 0;
    }

    delay(3);
}
