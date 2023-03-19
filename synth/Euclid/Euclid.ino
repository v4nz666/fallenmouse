/**********************
 * Encoder setup
 */
static int pinA = 2;             // Our first hardware interrupt pin is digital pin 2
static int pinB = 3;             // Our second hardware interrupt pin is digital pin 3
volatile byte aFlag = 0;         // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0;         // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile int8_t encoderPos = 0;  //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte if you want to record a larger range than 0-255
volatile byte reading = 0;       //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent


/**********************
 * Seven Segment Display
 */
#include "SevSeg.h"
SevSeg sevseg;
static const byte sixteen = 0b01110110;
static const byte dot = 0b10000000;

static const uint8_t digits[17] = {
  // GFEDCBA  Segments      7-segment map:
  0b00111111,  // 0   "0"          AAA
  0b00000110,  // 1   "1"         F   B
  0b01011011,  // 2   "2"         F   B
  0b01001111,  // 3   "3"          GGG
  0b01100110,  // 4   "4"         E   C
  0b01101101,  // 5   "5"         E   C
  0b01111101,  // 6   "6"          DDD
  0b00000111,  // 7   "7"
  0b01111111,  // 8   "8"
  0b01101111,  // 9   "9"
  0b01110111,  // 10  'A'
  0b01111100,  // 11  'b'
  0b00111001,  // 12  'C'
  0b01011110,  // 13  'd'
  0b01111001,  // 14  'E'
  0b01110001,  // 15  'F'
  0b01110110,  // 16  'H'
};

/**********************
 * Encoder Push Button / UI
 */
const int buttonPin = 4;
bool oldButtonState;
uint16_t debounceTimer = 0;
const static uint16_t debounceLength = 500;

byte currentParam = 0;

/**********************
 * State
 */

#include "Euclid.h";

const uint8_t totalChannels = 4;
int8_t currentChannel = 0;

struct Pattern {
  byte currentStep = 0;
  byte steps = 1;
  byte hits = 0;
  byte offset = 0;
  int16_t pattern;
};

Pattern patterns[totalChannels];

const byte maxSteps = 16;
const byte maxHits = 16;
const byte maxOffset = 15;

/**
 * Outputs
 */
const int OUTPUTS[4] = { 0, 1, 13, A0 };

const int CLK = A5;
bool lastClock;

void setup() {

  pinMode(pinA, INPUT_PULLUP);       // set pinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  pinMode(pinB, INPUT_PULLUP);       // set pinB as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  attachInterrupt(0, PinA, RISING);  // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (below)
  attachInterrupt(1, PinB, RISING);  // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (below)

  pinMode(buttonPin, INPUT_PULLUP);

  /**
   * Seven Segment setup (https://github.com/DeanIsMe/SevSeg)
   */
  byte numDigits = 4;
  byte digitPins[] = { A4, A3, A2, A1 };  //left - right
  byte segmentPins[] = { 5, 6, 7, 8, 9, 10, 11, 12 };
  bool resistorsOnSegments = false;      // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_CATHODE;  // See README.md for options
  bool updateWithDelays = false;         // Default 'false' is Recommended
  bool leadingZeros = true;              // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false;          // Use 'true' if your decimal point doesn't exist or isn't connected. Then, you only need to specify 7 segmentPins[]

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(0);
  sevseg.blank();

  for (byte i = 0; i < totalChannels; i++) {
    pinMode(OUTPUTS[i], OUTPUT);
    digitalWrite(OUTPUTS[i], LOW);
  }

  pinMode(CLK, INPUT);

  updateSequences();
  updateDisplay();
}


void loop() {

  /**
   * Have we recived a rising or falling edge on the clock pin?
   */
  bool currentClock = digitalRead(CLK);

  // Trigger on the rising edge of our clock signal
  if (currentClock && !lastClock) {
    tick();
  } else if (lastClock && !currentClock) {
    closeGates();
  }
  lastClock = currentClock;

  bool needsUpdate = false;
  if (debounceTimer > 0) {
    debounceTimer--;
    Serial.println("debounce");
  } else {
    bool newState = digitalRead(buttonPin) == LOW;
    if (!oldButtonState && newState) {
      debounceTimer = debounceLength;
      currentParam++;
      currentParam %= 4;
      Serial.println(currentParam);
      needsUpdate = true;
    }

    oldButtonState = newState;
  }
  if (encoderPos != 0) {
    needsUpdate = updateParams() || needsUpdate;
  }
  encoderPos = 0;

  if (needsUpdate) {
    updateSequences();
    updateDisplay();
  }

  sevseg.refreshDisplay();
}

bool updateParams() {
  if (encoderPos < 0) {
    // Left turn
    switch (currentParam) {
      case 0:
        currentChannel--;
        if (currentChannel < 0) currentChannel = totalChannels - 1;
        break;
      case 1:
        if (patterns[currentChannel].steps > 1)
          patterns[currentChannel].steps--;
        if (patterns[currentChannel].hits > patterns[currentChannel].steps)
          patterns[currentChannel].hits = patterns[currentChannel].steps;
        if (patterns[currentChannel].offset > patterns[currentChannel].steps - 1)
          patterns[currentChannel].offset = patterns[currentChannel].steps - 1;
        break;
      case 2:
        if (patterns[currentChannel].hits > 0)
          patterns[currentChannel].hits--;
        break;
      case 3:
        if (patterns[currentChannel].offset > 0)
          patterns[currentChannel].offset--;
        break;
    }
    return true;

  } else if (encoderPos > 0) {
    // Right turn
    switch (currentParam) {
      case 0:
        currentChannel++;
        currentChannel %= totalChannels;
        break;
      case 1:
        patterns[currentChannel].steps++;
        if (patterns[currentChannel].steps > maxSteps) patterns[currentChannel].steps = maxSteps;
        break;
      case 2:
        patterns[currentChannel].hits++;
        if (patterns[currentChannel].hits > patterns[currentChannel].steps)
          patterns[currentChannel].hits = patterns[currentChannel].steps;
        break;
      case 3:
        patterns[currentChannel].offset++;
        if (patterns[currentChannel].offset > patterns[currentChannel].steps - 1)
          patterns[currentChannel].offset = patterns[currentChannel].steps - 1;
        break;
    }
    return true;
  }

  return false;  // No update...
}

/**
 * Update the display with the current channel/sequence data
 */
void updateDisplay() {
  byte segs[4] = { digits[currentChannel + 1], digits[patterns[currentChannel].steps], digits[patterns[currentChannel].hits], digits[patterns[currentChannel].offset] };
  segs[currentParam] |= dot;

  sevseg.setSegments(segs);
}

/**
 * Regenerate the sequence for the current channel
 */
void updateSequences() {
  patterns[currentChannel].pattern = euclid(patterns[currentChannel].steps, patterns[currentChannel].hits, patterns[currentChannel].offset);
}

/**
 * Next sequence step
 */
void tick() {
  for (byte i = 0; i < totalChannels; i++) {
    bool on = getBit(patterns[i].currentStep, i);
    if (on) {
      digitalWrite(OUTPUTS[i], HIGH);
    }
    patterns[i].currentStep++;
    if (patterns[i].currentStep >= patterns[i].steps) {
      patterns[i].currentStep = 0;
    }
  }
}

/**
 * Close our gates
 */
void closeGates() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(OUTPUTS[i], LOW);
  }
}

/**
 * Return the bit from this step in our sequence
 */
bool getBit(byte step, byte channel) {
  return patterns[channel].pattern & (1 << (patterns[channel].steps - 1 - step));
}



/*******Interrupt-based Rotary Encoder Sketch*******
by Simon Merrett, based on insight from Oleg Mazurov, Nick Gammon, rt, Steve Spence
*/

void PinA() {
  cli();                                //stop interrupts happening before we read pin values
  reading = PIND & 0xC;                 // read all eight pin values then strip away all but pinA and pinB's values
  if (reading == B00001100 && aFlag) {  //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos--;                       //decrement the encoder's position count
    bFlag = 0;                          //reset flags for the next turn
    aFlag = 0;                          //reset flags for the next turn

  } else if (reading == B00000100) bFlag = 1;  //signal that we're expecting pinB to signal the transition to detent from free rotation
  sei();                                       //restart interrupts
}

void PinB() {
  cli();                                       //stop interrupts happening before we read pin values
  reading = PIND & 0xC;                        //read all eight pin values then strip away all but pinA and pinB's values
  if (reading == B00001100 && bFlag) {         //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    encoderPos++;                              //increment the encoder's position count
    bFlag = 0;                                 //reset flags for the next turn
    aFlag = 0;                                 //reset flags for the next turn
  } else if (reading == B00001000) aFlag = 1;  //signal that we're expecting pinA to signal the transition to detent from free rotation
  sei();                                       //restart interrupts
}