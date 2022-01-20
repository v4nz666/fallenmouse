//#define DEBUG

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

int sixteenthsPerEighth;
int eighthsPerQuarter;
int quartersPerBar;
int last_sixteenthsPerEighth;
int last_eighthsPerQuarter;
int last_quartersPerBar;

int sixteenthsPerBeat;
int sixteenthsPerBar;

int cycleTime;

SimpleTimer timer;

int count = 0;
bool started = false;
bool first = true;


float BPM;
float lastBpm = 0;

float dutyCycle;
float lastDutyCycle = 0;

int max_BPM = 240;
int min_BPM = 60;

#ifdef DEBUG
  unsigned long lastEight = 0;
#endif

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
  #ifdef DEBUG
    Serial.begin(9600);
  #endif
  
  pinMode(BPM_IN, INPUT);
  pinMode(DUTY_IN, INPUT);

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
  
  gatesOff();
  checkDivisionToggles();
  readPots();
  intro();
}

void loop() {
  if (!started) {
    started = true;
    tick();
  }
  timer.run();
}

void reset() {
  started = false;
  count = 0;
  
  gatesOff();
  updateLED(666);
  delay(1000);

  while ( digitalRead(RESET) == LOW ) {
    delay(10);
  }
  updateLED(BPM);
}

void resetDivs() {
  int divs = 100*sixteenthsPerEighth + 10*eighthsPerQuarter + quartersPerBar;
  updateLED(divs);
  //delay(1000);
  //reset();
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

  sixteenthsPerBeat = (eighthsPerQuarter * sixteenthsPerEighth);
  sixteenthsPerBar = sixteenthsPerBeat * quartersPerBar;

  #ifdef DEBUG
//    Serial.print("Checked toggles: ");
//    Serial.print("spe:");
//    Serial.print(sixteenthsPerEighth);
//    Serial.print(" epq:");
//    Serial.print(eighthsPerQuarter);
//    Serial.print(" spq:");
//    Serial.print(sixteenthsPerBeat);
//    Serial.print(" spb:");
//    Serial.println(sixteenthsPerBar);
  #endif
  
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

  cycleTime = (60000 / BPM) / sixteenthsPerBeat;
  timer.setTimeout(cycleTime, tick);

  if ( !started ) {
    return;
  }
  
  if ( digitalRead(RESET) == LOW ) {
    reset();
    return;
  }
  
  if ( checkDivisionToggles() ) {
    resetDivs();
  }

  digitalWrite(SIXTEENTH, HIGH);
  timer.setTimeout(scheduleOff(1, startTime), sixteenthOff);
  
  if ( count % sixteenthsPerEighth == 0 ) {
    digitalWrite(EIGHTH, HIGH);
    timer.setTimeout(scheduleOff(sixteenthsPerEighth, startTime), eighthOff);
  }
  
  if ( count % sixteenthsPerBeat == 0 ) {
    //Serial.print("Beat. Count: "); Serial.println(count);
    digitalWrite(QUARTER, HIGH);
    digitalWrite(BLINKER, HIGH);
    timer.setTimeout(scheduleOff(sixteenthsPerBeat, startTime), quarterOff);
  }
  
  if ( count % (sixteenthsPerBar / 2) == 0 ) {
    digitalWrite(HALF, HIGH);
    timer.setTimeout(scheduleOff(sixteenthsPerBar / 2, startTime), halfOff);
  }
  
  if ( count % (sixteenthsPerBar) == 0 ) {
    digitalWrite(WHOLE, HIGH);
    timer.setTimeout(scheduleOff(sixteenthsPerBar, startTime), wholeOff);
  }
  
  if ( count % (sixteenthsPerBar * 2) == 0 ) {
    digitalWrite(TWO, HIGH);
    timer.setTimeout(scheduleOff(sixteenthsPerBar * 2, startTime), twoOff);
  }
  
  if ( count % (sixteenthsPerBar * 4) == 0 ) {
    digitalWrite(FOUR, HIGH);
    timer.setTimeout(scheduleOff(sixteenthsPerBar * 4, startTime), fourOff);
  }
  
  if ( count == 0 ) {
    digitalWrite(EIGHT, HIGH);
    unsigned int _delay = scheduleOff(sixteenthsPerBar * 8, startTime);
    timer.setTimeout(_delay, eightOff);
  }

  readPots();
  
  if (dutyCycle != lastDutyCycle) {
    updateLED((int)dutyCycle);
    lastDutyCycle = dutyCycle;
  } else if (lastBpm != BPM || count % sixteenthsPerBar == 0) {
    lastBpm = BPM;
    updateLED(BPM);
  }
  
  count++;
  if ( count == sixteenthsPerBar * 8 ) {
    count = 0;
  }
}

void readPots() {
  BPM = map(analogRead(BPM_IN), 0, 1023, min_BPM, max_BPM);
  dutyCycle = map(analogRead(DUTY_IN), 0, 1023, 1, 90);
}

void sixteenthOff() {
  digitalWrite(SIXTEENTH, LOW);
}
void eighthOff() {
  digitalWrite(EIGHTH, LOW);
}
void quarterOff() {
  digitalWrite(QUARTER, LOW);
}
void halfOff() {
  digitalWrite(HALF, LOW);
}
void wholeOff() {
  digitalWrite(WHOLE, LOW);
}
void twoOff() {
  digitalWrite(TWO, LOW);
}
void fourOff() {
  digitalWrite(FOUR, LOW);
}
void eightOff() {
  #ifdef DEBUG
    Serial.println("8-bar Off");
  #endif

  digitalWrite(EIGHT, LOW);
}

float scheduleOff(int multiplier, int startTime) {
  int diff = millis() - startTime;
  int _delay = (cycleTime * multiplier * (dutyCycle / 100)) - diff;
  #ifdef DEBUG
    Serial.print("Count: ");
    Serial.print(count);
    Serial.print(" Multiplier: ");
    Serial.print(multiplier);
    Serial.print(" Diff: ");
    Serial.print(diff);
    Serial.print(" Calculated Delay: ");
    Serial.print(_delay);
    Serial.print("ms. cycleTime: ");
    Serial.println(cycleTime);
  #endif
  return _delay;
}

/**
 * turn off all gates
 */
void gatesOff() {
  #ifdef DEBUG
    Serial.print("Setting all gates to LOW");
  #endif

  digitalWrite(SIXTEENTH, LOW);
  digitalWrite(EIGHTH, LOW);
  digitalWrite(QUARTER, LOW);
  digitalWrite(BLINKER, LOW);
  digitalWrite(HALF, LOW);
  digitalWrite(WHOLE, LOW);
  digitalWrite(TWO, LOW);
  digitalWrite(FOUR, LOW);
  digitalWrite(EIGHT, LOW);
  
  #ifdef DEBUG
    Serial.println();
  #endif
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
  updateLED(669);
  delay(250);
  updateLED(696);
  delay(250);
  updateLED(966);
  delay(250);
  updateLED(666);
  delay(250);
  updateLED(669);
  delay(250);
  updateLED(696);
  delay(250);
  updateLED(966);
  delay(250);
  updateLED(666);
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
