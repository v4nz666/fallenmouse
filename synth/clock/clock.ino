#include <SimpleTimer.h>

const int BPM_IN = A7;
const int DUTY_IN = A6;
const int CLK = A5;
const int DATA = A4;

const int SIXTEENTHS_PER_EIGHTH_IN = A3;
const int EIGHTHS_PER_QUARTER_IN = A2;
const int QUARTERS_PER_BAR_IN = A1;

const int SIXTEENTH = 2;
const int EIGHTH = 3;
const int QUARTER = 4;
const int HALF = 5;
const int WHOLE = 6;
const int TWO = 7;
const int FOUR = 8;
const int EIGHT = 9;

const int RESET = 10;

const int BLINKER = 13;
bool beat;

int sixteenthsPerEighth;
int eighthsPerQuarter;
int quartersPerBar;
int last_sixteenthsPerEighth;
int last_eighthsPerQuarter;
int last_quartersPerBar;

int sixteenthsPerBeat;
int sixteenthsPerBar;

SimpleTimer timer;

int count = 0;
bool started = false;
bool first = true;


float BPM;
float lastBpm = 0;

float dutyCycle;
float lastDutyCycle = 0;

int max_BPM = 282;
int min_BPM = 90;


const bool blank[8] = {0, 0, 0, 0, 0, 0, 0, 0};
const bool digits[10][8] = {
  {1, 1, 1, 1, 1, 1, 0, 0}, //0
  {0, 1, 1, 0, 0, 0, 0, 0}, //1
  {1, 1, 0, 1, 1, 0, 1, 0}, //2
  {1, 1, 1, 1, 0, 0, 1, 0}, //3
  {0, 1, 1, 0, 0, 1, 1, 0}, //4
  {1, 0, 1, 1, 0, 1, 1, 0}, //5
  {0, 0, 1, 1, 1, 1, 1, 0}, //6
  {1, 1, 1, 0, 0, 0, 0, 0}, //7
  {1, 1, 1, 1, 1, 1, 1, 0}, //8
  {1, 1, 1, 0, 0, 1, 1, 0}  //9
};

void setup() {
  Serial.begin(9600);

  pinMode(BPM_IN, INPUT);

  pinMode(CLK, OUTPUT);
  pinMode(DATA, OUTPUT);
  
  pinMode(SIXTEENTH, OUTPUT);
  pinMode(EIGHTH, OUTPUT);
  pinMode(QUARTER, OUTPUT);
  pinMode(HALF, OUTPUT);
  pinMode(WHOLE, OUTPUT);
  pinMode(TWO, OUTPUT);
  pinMode(FOUR, OUTPUT);
  pinMode(EIGHT, OUTPUT);

  pinMode(SIXTEENTHS_PER_EIGHTH_IN, INPUT_PULLUP);
  pinMode(EIGHTHS_PER_QUARTER_IN, INPUT_PULLUP);
  pinMode(QUARTERS_PER_BAR_IN, INPUT_PULLUP);

  pinMode(RESET, INPUT_PULLUP);
  intro();

}

void loop() {
  //Serial.print("loop "); Serial.println(started ? "true": "false");
  if (!started) {
    started = true;
    tick();
  }

  timer.run();
}

void reset() {
  
  tock();
  updateLED(666);
  
  started = false;
  count = 0;

  delay(1000);

  while ( digitalRead(RESET) == LOW ) {
    delay(10);
  }
  updateLED(BPM);
  
}

void resetDivs() {
  int divs = 100*sixteenthsPerEighth + 10*eighthsPerQuarter + quartersPerBar;
  updateLED(divs);
  delay(1000);
  reset();
}

bool checkDivisionToggles() {
  bool divsChanged = false;
  
  if ( digitalRead(SIXTEENTHS_PER_EIGHTH_IN) == LOW ) {
    sixteenthsPerEighth = 3;
  } else {
    sixteenthsPerEighth = 2;
  }
  
  if ( last_sixteenthsPerEighth != sixteenthsPerEighth ) {
      divsChanged = true;
      last_sixteenthsPerEighth = sixteenthsPerEighth;
  }
  
  if ( digitalRead(EIGHTHS_PER_QUARTER_IN) == LOW ) {
    eighthsPerQuarter = 3;
  } else {
    eighthsPerQuarter = 2;
  }
  
  if ( last_eighthsPerQuarter != eighthsPerQuarter ) {
      divsChanged = true;
      last_eighthsPerQuarter = eighthsPerQuarter;
  }
  
  if ( digitalRead(QUARTERS_PER_BAR_IN) == LOW ) {
    quartersPerBar = 3;
  } else {
    quartersPerBar = 4;
  }

  if ( last_quartersPerBar != quartersPerBar ) {
      divsChanged = true;
      last_quartersPerBar = quartersPerBar;
  }
  return divsChanged;
}

/**********************************
 * Main timer functions
 */

/**
 * tick - turn on the appropriate gates for the current cycle
 */
void tick() {
  int startTime = millis();

  if ( !started ) {
    //Serial.println("Aborting");
    return;
  }
  
  if ( digitalRead(RESET) == LOW ) {
    reset();
    return;
  }
  
  bool divsChanged = checkDivisionToggles();
  
  if ( divsChanged ) {
    sixteenthsPerBeat = (eighthsPerQuarter * sixteenthsPerEighth);
    sixteenthsPerBar = sixteenthsPerBeat * quartersPerBar;
    resetDivs();
    //Serial.println("DivsChanged");
    return;
  }
  
  
  digitalWrite(SIXTEENTH, HIGH);
  if ( count % sixteenthsPerEighth == 0 ) {
    digitalWrite(EIGHTH, HIGH);
  }
  if ( count % sixteenthsPerBeat == 0 ) {
    //Serial.print("Beat. Count: "); Serial.println(count);
    beat = true;
    digitalWrite(QUARTER, HIGH);
    digitalWrite(BLINKER, HIGH);
  }
  if ( count % (sixteenthsPerBar / 2) == 0 ) {
    digitalWrite(HALF, HIGH);
  }
  if ( count % (sixteenthsPerBar) == 0 ) {
    digitalWrite(WHOLE, HIGH);
  }
  if ( count % (sixteenthsPerBar * 2) == 0 ) {
    digitalWrite(TWO, HIGH);
  }
  if ( count % (sixteenthsPerBar * 4) == 0 ) {
    digitalWrite(FOUR, HIGH);
  }
  if ( count == 0 ) {
    digitalWrite(EIGHT, HIGH);
  }

  BPM = map(analogRead(BPM_IN), 0, 1023, min_BPM, max_BPM);
  int cycleTime = (60000 / BPM) / sixteenthsPerBeat - (millis() - startTime);

  dutyCycle = map(analogRead(DUTY_IN), 0, 1023, 1, 90);
  if (dutyCycle != lastDutyCycle) {
    updateLED((int)dutyCycle);
    lastDutyCycle = dutyCycle;
  } else if (lastBpm != BPM || count % sixteenthsPerBar == 0) {
    lastBpm = BPM;
    updateLED(BPM);
  }

  // Subtract the time it took to get through this function from our delay
  int diff = millis() - startTime;
  Serial.println(diff);
  float nextStartDelay = cycleTime - diff;
  float nextStopDelay = (cycleTime * (dutyCycle / 100) - diff);
  
  timer.setTimeout(nextStartDelay, tick);
  timer.setTimeout(nextStopDelay, tock);
}

/**
 * tock - turn off all gates
 */
void tock() {
  digitalWrite(SIXTEENTH, LOW);
  digitalWrite(EIGHTH, LOW);
  digitalWrite(QUARTER, LOW);
  digitalWrite(HALF, LOW);
  digitalWrite(WHOLE, LOW);
  digitalWrite(TWO, LOW);
  digitalWrite(FOUR, LOW);
  digitalWrite(EIGHT, LOW);
  if ( beat ) {
    digitalWrite(BLINKER, LOW);
    beat = false;
  }
  count++;

  if ( count == sixteenthsPerBar * 8 ) {
    count = 0;
  }
}

/***
   /Timer functions
 ******************************************************/

/******************************************************
   7-digit display interface
*/

void intro() {
  digitalWrite(CLK, LOW);
  updateLED(666);
  delay(250);
  updateLED(999);
  delay(250);
  updateLED(666);
  delay(250);
  updateLED(999);
  delay(250);
  updateLED(666);
  delay(250);
  updateLED(999);
  delay(250);
  updateLED(666);
  delay(250);
  updateLED(999);
  delay(250);
}

/**
   Display the given 2- or 3-digit number on the 7 segment display.
*/
void updateLED(int bpm) {
  int digitOne = bpm / 100;
  int digitTwo = bpm % 100 / 10;
  int digitThree = bpm % 10;

  if ( digitOne == 0 ) {
    digitOne = -1;
    if ( digitTwo == 0 ) {
      digitTwo = -1;
    }
  }

  // Send initialization bit
  sendBit(1);

  writeDigit(digitOne);
  writeDigit(digitTwo);
  writeDigit(digitThree);

  for ( int i = 0; i < 11; i++ ) {
    sendBit(0);
  }
}

/**
   Send a single bit to the display
*/
void sendBit(bool b) {
  digitalWrite(DATA, b);
  digitalWrite(CLK, HIGH);
  digitalWrite(CLK, LOW);
}

/**
   Write a digit to the display's data stream
*/
void writeDigit(int digit) {
  bool bits[8];
  if ( digit == -1 ) {
    memcpy(bits, blank, 8);
  } else {
    memcpy(bits, digits[digit], 8);
  }
  for ( int i = 0; i < 8; i++ ) {
    sendBit(bits[i]);
  }
}
