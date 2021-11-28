/*******************************
 * PINS
 */
/**
 * Breadboard:
 */                      // MCP4902:
const int CS_DAC = 10;   // pin3 
const int CLK_DAC = 11;  // pin4
const int DATA_DAC = 12; // pin5
                         // CD4015:
const int CLK_REG = 2;   // pins 1/9
const int DATA_REG = 3;  // pin 7

const int INA = A0;
const int INB = A1;

const int SCALE_SEL = 4;

/*
 *******************************/

const int DAC_A = LOW;
const int DAC_B = HIGH;

const float semiTone = 16.787;

const int debounceDelay = 250;
int debounceTimer = 0;

const int scales[16][12] = {
  {1,0,1,0,1,1,0,1,0,1,0,1}, // Ionian/Major
  {1,0,1,1,0,1,0,1,0,1,1,0}, // Dorian
  {1,1,0,1,0,1,0,1,1,0,1,0}, // Phrygian
  {1,0,1,0,1,0,1,1,0,1,0,1}, // Lydian
  {1,0,1,0,1,1,0,1,0,1,1,0}, // Mixolydian
  {1,0,1,1,0,1,0,1,1,0,1,0}, // Aeolian/Minor
  {1,1,0,1,0,1,1,0,1,0,1,0}, // Locrian
  {1,0,1,1,0,1,0,1,1,0,0,1}, // Harmonic Minor
  {1,0,1,1,0,0,1,1,0,1,1,0}, // Ukrainian Minor
  {1,1,0,0,1,1,0,1,1,0,1,0}, // Phrygian Dominant
  {1,0,1,1,0,0,1,1,1,0,0,1}, // Hungarian Minor
  {1,0,1,0,1,0,0,1,0,1,0,0}, // Maj Pentatonic
  {1,0,0,1,0,1,0,1,0,0,1,0}, // Min Pentatonic
  {1,0,0,1,0,1,1,1,0,0,1,0}, // Blues
  {1,1,1,1,1,1,1,1,1,1,1,1}, // Chromatic
  {1,0,1,0,1,0,1,0,1,0,1,0}, // Whole tone
};

const int numbers[16][8] = {
  {1,1,1,1,1,1,0,0},
  {0,1,1,0,0,0,0,0},
  {1,1,0,1,1,0,1,0},
  {1,1,1,1,0,0,1,0},
  {0,1,1,0,0,1,1,0},
  {1,0,1,1,0,1,1,0},
  {1,0,1,1,1,1,1,0},
  {1,1,1,0,0,0,0,0},
  {1,1,1,1,1,1,1,0},
  {1,1,1,0,0,1,1,0},
  {1,1,1,0,1,1,1,0},
  {0,0,1,1,1,1,1,0},
  {1,0,0,1,1,1,0,0},
  {0,1,1,1,1,0,1,0},
  {1,0,0,1,1,1,1,0},
  {1,0,0,0,1,1,1,0}
};

int currentScale = 0;
bool scaleUpdated = true;

/**
 * Setup pinModes, initialize SPI pins to known state
 */
void setup() {
  Serial.begin(9600);

  pinMode(INA, INPUT);
  pinMode(INB, INPUT);

  pinMode(CS_DAC, OUTPUT);
  pinMode(CLK_DAC, OUTPUT);
  pinMode(DATA_DAC, OUTPUT);

  pinMode(CLK_REG, OUTPUT);
  pinMode(DATA_REG, OUTPUT);
  
  pinMode(SCALE_SEL, INPUT_PULLUP);

  
  digitalWrite(CS_DAC, HIGH);
  digitalWrite(CLK_DAC, LOW);
  digitalWrite(DATA_DAC, LOW);
  
}

void loop() {

  checkScale();
  if ( scaleUpdated ) {
    outputScale();
  }
  
  toDAC(DAC_A, INA);
  toDAC(DAC_B, INB);
  
}

/**
 * Check if the scale-select button has been pressed, and update selected scale accordingly
 */
void checkScale() {
  
  if (debounceTimer) {
    debounceTimer--;
    return;
  }
  
  if ( digitalRead(SCALE_SEL) == LOW ) {
    debounceTimer = debounceDelay;
    currentScale++;
    currentScale %= 16;
    scaleUpdated = true;
  }
}

/**
 * Update the scale selection LEDs 
 */
void outputScale() {
  scaleUpdated = false;
  for ( int x = 0; x < 8; x++ ) {
    regWrite(numbers[currentScale][x]);
  }
}

void dacWrite(int val) {
  spiWrite(val, CLK_DAC, DATA_DAC);
}

void regWrite(int val) {
  spiWrite(val, CLK_REG, DATA_REG);
}
/**
 * Toggle clockPin pin and write a bit to dataPin 
 */
void spiWrite(int val, int clkPin, int dataPin) {  
  digitalWrite(clkPin, LOW);
  digitalWrite(dataPin, val);
  digitalWrite(clkPin, HIGH);
}

/**
 * Quantize a note to our currently selected scale
 */
int quantize(int note){
  int original = note;
  bool reverse = false;
  while ( !scales[currentScale][note % 12] && !reverse) {
    note++;
    if ( note > 60 ) {
      note = original;
      reverse = true;
    }
  }
  if ( reverse ) {
    while ( !scales[currentScale][note % 12] && !reverse) {
      note--;
      // We just can't win... just return the original
      if ( note < 0 ) {
        return original;
      }
    }
  }

  return note;
}

/**
 * Given an analog input, return a quantized note (0-60)
 */
int getNote(int in) {
  // Shift value up by half of the to account for integer division flooring the result below
  float adjusted = in + (semiTone / 2);
  int note = adjusted / semiTone;
  
  return quantize(note);
}

void toDAC(int select, int pin) {
  
  int input = analogRead(pin);
  int note = getNote(input);
  
  digitalWrite(CS_DAC, LOW);
  dacWrite(select);
  dacWrite(LOW); // Unbuffered input
  dacWrite(HIGH); // Gain 1x
  dacWrite(HIGH); // Active(high)/shutdown
  dacWrite((note & 32) >> 5);
  dacWrite((note & 16) >> 4);
  dacWrite((note & 8) >> 3);
  dacWrite((note & 4) >> 2);
  dacWrite((note & 2) >> 1);
  dacWrite((note & 1) >> 0);
  dacWrite(LOW);
  dacWrite(LOW);
  dacWrite(LOW);
  dacWrite(LOW);
  dacWrite(LOW);
  dacWrite(LOW);
  digitalWrite(CS_DAC, HIGH);
}
