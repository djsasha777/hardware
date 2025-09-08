// Teensy-4.0 TalkBox-MidiController
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>

// Audio Shield pins comments omitted for brevity

// pots
#define NUM_POTS 9
#define POT_RANGE 1023

int potPins[NUM_POTS] = {A0, A1, A2, A3, A8, A10, A11, A12, A13};
int currentPositions[NUM_POTS];
int previousValues[NUM_POTS];

// Motor control pins (pairs: IN1, IN2 for each motor)
// Must support PWM outputs on Teensy 4.0
const int motorPins[NUM_POTS][2] = {
  {0, 1},    // Motor 1
  {2, 3},    // Motor 2
  {4, 5},    // Motor 3
  {6, 9},    // Motor 4
  {10, 11},  // Motor 5
  {12, 28},  // Motor 6
  {29, 33},  // Motor 7
  {34, 35},  // Motor 8
  {36, 37}   // Motor 9
};

// Max PWM for Teensy 4.0
#define MAX_PWM 255

// MIDI CC control numbers
#define CCmixer 20
#define CCattack 21
#define CCdecay 22
#define CCsustain 23
#define CCrelease 24
#define CCosc 25
#define CCfilterfreq 26
#define CCfilterres 27
#define CCbendrange 28

// Audio objects
AudioSynthWaveform       waveform1;
AudioMixer4              mixer1;
AudioFilterStateVariable filter1;
AudioEffectEnvelope      envelope1;
AudioOutputI2S           i2s1;
AudioConnection          patchCord5(waveform1, 0, mixer1, 0);
AudioConnection          patchCord6(mixer1, 0, filter1, 0);
AudioConnection          patchCord9(filter1, 0, envelope1, 0);
AudioConnection          patchCord10(envelope1, 0, i2s1, 0);
AudioConnection          patchCord11(envelope1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;

// GLOBAL VARIABLES
const byte BUFFER = 8;
const float noteFreqs[] = {
  8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.25, 12.978, 13.75,
  14.568, 15.434, 16.352, 17.324, 18.354, 19.445, 20.602, 21.827, 23.125, 24.5,
  25.957, 27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654,
  46.249, 48.999, 51.913, 55, 58.27, 61.735, 65.406, 69.296, 73.416, 77.782,
  82.407, 87.307, 92.499, 97.999, 103.826, 110, 116.541, 123.471, 130.813,
  138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652,
  220, 233.082, 246.942, 261.626, 277.183, 293.665, 311.127, 329.628, 349.228,
  369.994, 391.995, 415.305, 440, 466.164, 493.883, 523.251, 554.365, 587.33,
  622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880, 932.328, 987.767,
  1046.502, 1108.731, 1174.659, 1244.508, 1318.51, 1396.913, 1479.978, 1567.982,
  1661.219, 1760, 1864.655, 1975.533, 2093.005, 2217.461, 2349.318, 2489.016,
  2637.02, 2793.826, 2959.955, 3135.963, 3322.438, 3520, 3729.31, 3951.066,
  4186.009, 4434.922, 4698.636, 4978.032, 5274.041, 5587.652, 5919.911,
  6271.927, 6644.875, 7040, 7458.62, 7902.133, 8372.018, 8869.844, 9397.273,
  9956.063, 10548.08, 11175.3, 11839.82, 12543.85
};
byte globalNote = 0;
byte globalVelocity = 0;
const float DIV127 = (1.0 / 127.0);
const float DIV1023 = (1.0 / 1023.0);
float bendFactor = 1;
int bendRange = 12;
int FILfreq =  10000;
float FILfactor = 1;

void setup() {
  Serial.begin(9600);
  AudioMemory(20);
  usbMIDI.setHandleControlChange(myControlChange);
  usbMIDI.setHandleNoteOff(myNoteOff);
  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandlePitchChange(myPitchBend);
  Serial.println("MIDI init done");
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.9);
  waveform1.begin(WAVEFORM_SAWTOOTH);
  waveform1.amplitude(0.75);
  waveform1.frequency(82.41);
  waveform1.pulseWidth(0.15);
  mixer1.gain(0, 1.0);
  envelope1.attack(0);
  envelope1.decay(0);
  envelope1.sustain(1);
  envelope1.release(500);
  Serial.println("AUDIO init done");

  for (int i = 0; i < NUM_POTS; ++i) {
    pinMode(motorPins[i][0], OUTPUT);
    pinMode(motorPins[i][1], OUTPUT);
    digitalWrite(motorPins[i][0], LOW);
    digitalWrite(motorPins[i][1], LOW);
    previousValues[i] = -1; // init previousValues to invalid to detect first change
  }
}

void loop() {
  usbMIDI.read();
  checkPots();
}

void myNoteOn(byte channel, byte note, byte velocity) {
  if (note > 23 && note < 108) {
    globalNote = note;
    globalVelocity = velocity;
    keyBuff(note, true);
  }
}

void myNoteOff(byte channel, byte note, byte velocity) {
  if (note > 23 && note < 108) {
    keyBuff(note, false);
  }
}

void myPitchBend(byte channel, int bend) {
  float bendF = (float)bend / 8192.0 * bendRange / 12.0;
  bendFactor = pow(2.0, bendF);
  oscSet();
}

void keyBuff(byte note, bool playNote) {
  static byte buff[BUFFER];
  static byte buffSize = 0;

  if (playNote && buffSize < BUFFER) {
    // Add note
    oscPlay(note);
    buff[buffSize++] = note;
    return;
  }

  if (!playNote && buffSize > 0) {
    // Remove note
    for (byte found = 0; found < buffSize; found++) {
      if (buff[found] == note) {
        for (byte gap = found; gap < buffSize - 1; gap++) {
          buff[gap] = buff[gap + 1];
        }
        buffSize--;
        buff[buffSize] = 255;
        if (buffSize > 0) {
          oscPlay(buff[buffSize - 1]);
          return;
        } else {
          oscStop();
          return;
        }
      }
    }
  }
}

void oscPlay(byte note) {
  waveform1.frequency(noteFreqs[note] * bendFactor);
  float velo = 0.75 * (globalVelocity * DIV127);
  waveform1.amplitude(velo);
  envelope1.noteOn();
  Serial.print("note - ");
  Serial.println(note);
  Serial.print("velo - ");
  Serial.println(velo);
}

void oscStop() {
  envelope1.noteOff();
}

void oscSet() {
  waveform1.frequency(noteFreqs[globalNote] * bendFactor);
}

void myControlChange(byte channel, byte control, byte value) {
  switch (control) {
    case CCmixer:
      myMotorControl(0, value);
      break;
    case CCattack:
      myMotorControl(1, value);
      break;
    case CCdecay:
      myMotorControl(2, value);
      break;
    case CCsustain:
      myMotorControl(3, value);
      break;
    case CCrelease:
      myMotorControl(4, value);
      break;
    case CCosc:
      myMotorControl(5, value);
      break;
    case CCfilterfreq:
      myMotorControl(6, value);
      break;
    case CCfilterres:
      myMotorControl(7, value);
      break;
    case CCbendrange:
      myMotorControl(8, value);
      break;
  }
}

void checkPots() {
  for (int i = 0; i < NUM_POTS; i++) {
    int currentValue = analogRead(potPins[i]);
    if (currentValue != previousValues[i]) {
      handlePotUpdate(i, currentValue);
      previousValues[i] = currentValue;
    }
  }
}

void handlePotUpdate(int index, int value) {
  switch (index) {
    case 0:
      mixer1.gain(0, 0.9 * (value * DIV1023));
      break;
    case 1:
      envelope1.attack((3000 * (value * DIV1023)) + 10.5);
      break;
    case 2:
      envelope1.decay(3000 * (value * DIV1023));
      break;
    case 3:
      envelope1.sustain(value * DIV1023);
      break;
    case 4:
      envelope1.release(3000 * (value * DIV1023));
      break;
    case 5:
      if (value <= 256) {
        waveform1.begin(WAVEFORM_SINE);
      } else if (value <= 512) {
        waveform1.begin(WAVEFORM_TRIANGLE);
      } else if (value <= 765) {
        waveform1.begin(WAVEFORM_SAWTOOTH);
      } else {
        waveform1.begin(WAVEFORM_PULSE);
      }
      break;
    case 6:
      FILfreq = 10000 * (value * DIV1023);
      break;
    case 7:
      filter1.resonance((4.3 * (value * DIV1023)) + 0.7);
      break;
    case 8:
      if (value <= 12 && value > 0) {
        bendRange = value;
      }
      break;
  }
}

void myMotorControl(int index, int value) {
  // Map MIDI CC (0-127) to motor angle (0-315 degrees)
  int targetPosition = map(value, 0, 127, 0, 315);

  // Read current position from potentiometer, map 0-1023 to 0-315 degrees
  int currentPosition = analogRead(potPins[index]) * 315 / 1023;

  Serial.print("motor - ");
  Serial.println(index);
  Serial.print("value - ");
  Serial.println(value);
  currentPositions[index] = currentPosition;

  int error = targetPosition - currentPosition;

  const int deadzone = 2;

  if (abs(error) <= deadzone) {
    stopMotor(index);
    return;
  }

  int speed = map(abs(error), 0, 315, 0, MAX_PWM);
  if (speed < 50) speed = 50;

  if (error > 0) {
    forwardMotor(index, speed);
  } else {
    backwardMotor(index, speed);
  }

  delay(10);
}

// Motor control for TC1508A driver: 
// motorPins[index][0] = IN1 (PWM)
// motorPins[index][1] = IN2 (Digital LOW/HIGH)

void forwardMotor(int index, int speed) {
  analogWrite(motorPins[index][0], speed);  // PWM signal
  digitalWrite(motorPins[index][1], LOW);   // IN2 LOW for forward
}

void backwardMotor(int index, int speed) {
  digitalWrite(motorPins[index][0], LOW);   // IN1 LOW
  analogWrite(motorPins[index][1], speed);  // PWM signal on IN2 for backward
}

void stopMotor(int index) {
  digitalWrite(motorPins[index][0], LOW);
  digitalWrite(motorPins[index][1], LOW);
}
