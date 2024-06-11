
const int CLK_PIN = 11;
const int RESET_PIN = 7;

const int CLK_DEBOUNCE_TIME = 10;
int prevClk = 0;
int clkDebounce = 0;

const int RESET_DEBOUNCE_TIME = 10;
int prevReset = 0;
int resetDebounce = 0;

// selected nr of steps
int div_steps[3] = {2, 4, 8};
// step counters
int div_count[3] = {0, 0, 0};


// wire colors: 0: purple, 1: orange, 2: green
// analog division selector input pins
const static int DIV_SELECT_PIN[3] = {0, 1, 2};
// division output pins
const static int DIV_OUT_PIN[3] = {8, 10, 9};

// each divider can select a different set of steps 
// using the DIV_SELECT_PIN input potentiometer
const static int SELECTABLE_STEPS[3][6] = {
  {2, 3, 4, 5, 6, 8},
  {2, 3, 4, 5, 6, 8},
  {4, 8, 16, 32, 64, 128},
};

void setup() {
  // put your setup code here, to run once:
  pinMode(DIV_SELECT_PIN[0], INPUT);
  pinMode(DIV_SELECT_PIN[1], INPUT);
  pinMode(DIV_SELECT_PIN[2], INPUT);

  pinMode(DIV_OUT_PIN[0], OUTPUT);
  pinMode(DIV_OUT_PIN[1], OUTPUT);
  pinMode(DIV_OUT_PIN[2], OUTPUT);

  pinMode(CLK_PIN, INPUT);
  pinMode(RESET_PIN, INPUT);

  prevClk = 0;
  clkDebounce = CLK_DEBOUNCE_TIME;

  prevReset = 0;
  resetDebounce = RESET_DEBOUNCE_TIME;
}

void updateDivision(int divIdx, int edge) {

  // update the output
  if (edge == 1) {
    div_count[divIdx] += 1;
    if (div_count[divIdx] >= div_steps[divIdx]) {
        div_count[divIdx] = 0;
        digitalWrite(DIV_OUT_PIN[divIdx], HIGH);
    }
  } else if (edge == -1) {
    digitalWrite(DIV_OUT_PIN[divIdx], LOW);
  } else {
    // no event. Lets see if we should update the number of steps

    // read the nr of steps from analog pin
    int val = analogRead(DIV_SELECT_PIN[divIdx]);
    int newSteps = 0;
    if (val <= 170) {
      newSteps = SELECTABLE_STEPS[divIdx][0];
    } else if (val <= 340) {
      newSteps = SELECTABLE_STEPS[divIdx][1];
    } else if (val <= 510) {
      newSteps = SELECTABLE_STEPS[divIdx][2];
    } else if (val <= 680) {
      newSteps = SELECTABLE_STEPS[divIdx][3];
    } else if (val <= 850) {
      newSteps = SELECTABLE_STEPS[divIdx][4];
    } else {
      newSteps = SELECTABLE_STEPS[divIdx][5];
    }
    if (newSteps != div_steps[divIdx]) {
      // When a change is detected, update number of steps in division & reset the clockdivider and restart
      div_steps[divIdx] = newSteps; 
      resetClockDivider();
    }
  }
}

void resetClockDivider() {
  // Set all counters to the current number of steps. This guarantees that on
  // the next clock input, all outputs emit a signal
  div_count[0] = div_steps[0];
  div_count[1] = div_steps[1];
  div_count[2] = div_steps[2];
}

int readClk() {
  // Returns 1 on rising edge and -1 on falling edge
  int clk = digitalRead(CLK_PIN);

  if (clkDebounce == 0) {
    // detect rising edge
    if (prevClk == LOW && clk == HIGH) {
      clkDebounce = CLK_DEBOUNCE_TIME;
      prevClk = HIGH;
      return 1;
    }
    // detect falling edge
    if (prevClk == HIGH && clk == LOW) {
      clkDebounce = CLK_DEBOUNCE_TIME;
      prevClk = LOW;
      return -1;
    }    
  } else {
    clkDebounce -= 1;
  }
  
  return 0;
}


int readReset() {
  // Returns 1 on rising edge and -1 on falling edge
  int reset = digitalRead(RESET_PIN);

  if (resetDebounce == 0) {
    // detect rising edge
    if (prevReset == LOW && reset == HIGH) {
      resetDebounce = RESET_DEBOUNCE_TIME;
      prevReset = HIGH;
      return 1;
    }
    // detect falling edge
    if (prevReset == HIGH && reset == LOW) {
      resetDebounce = RESET_DEBOUNCE_TIME;
      prevReset = LOW;
      return -1;
    }    
  } else {
    resetDebounce -= 1;
  }
  
  return 0;
}

void loop() {
  int resetEdge = readReset();
  if (resetEdge == HIGH) {
    // On rising edge, reset all counters
    resetClockDivider();
  }

  // We only process clock signals when the last read reset state is low
  if (prevReset == LOW) {
    int clkEdge = readClk();

    // process all 3 divisions
    for (int i=0; i<3; i++) {
      updateDivision(i, clkEdge);
    }
  }

}

