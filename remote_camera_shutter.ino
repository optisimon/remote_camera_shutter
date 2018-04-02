/*
 * Remote shutter controller - Panasonic DMC-G80, FZ20, and probably others
 *
 * Emulates a DMW-RS1
 *
 * Used circuit:
 *
 *                  _____             _____
 * 2.5mm pin4   ___|2k0  |____.______|2k7  |________________.__________.
 *                 |_____|    |      |_____|                |         _|_
 *                  _____     |                   _____     |        |36k|
 * arduino pin3 ___|330R |___|/   arduino pin2___|330R |___|/        |   |
 *                 |_____|   |\e                 |_____|   |\e       |_._|
 *                            | BC547B                      | BC547B   |
 * 2.5mm pin3   ______________|_____________________________|__________|
 *
 *           _______________________
 *   _ _ _ _| 2.5 mm remote shutter |________________
 *  (_|_|_|_| plug pinout diagram    ________________
 *   1 2 3 4|_______________________|
 *
 * NOTE: I could probably get the same effect without the NPN transistors, but
 *       it might be bad to assume that the remote shutter circuit in all
 *       Panasonic cameras are very low voltage.
 *         But it's likely possible to remove the 330 Ohm resistors, either
 *       relying on internal pullups, or maybe even the limited drive
 *       capability of the pins...
 *
 * Starts 1MHz oscillator on pin 9 when full press activated (for measuring
 * shutter lag). Only expected to work with an arduino uno r3.
 *
 *
 * Copyright (c) 2018 Simon Gustafsson (www.optisimon.com)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

const int freqOutputPin = 9;   // OC1A output pin for ATmega32u4 (Arduino Micro)
const int ocr1aval  = 7; // 7 for 1MHz, or 0 for 8MHz

const int halfPressPin = 2;
const int fullPressPin = 3;

// 1MHz counter
void setup() {
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Only needed for native USB port.
  }
  Serial.println("Connected:");
  pinMode(freqOutputPin, OUTPUT);
  digitalWrite(halfPressPin, LOW);
  digitalWrite(fullPressPin, LOW);
  pinMode(halfPressPin, OUTPUT);
  pinMode(fullPressPin, OUTPUT);
//  TCCR1A = ( (1 << COM1A0));
  TCCR1B = ((1 << WGM12) | (1 << CS10));
  TIMSK1 = 0;
  OCR1A = ocr1aval;
}

void runTestCycle(int halfpress_ms)
{
  digitalWrite(halfPressPin, HIGH);
  if (halfpress_ms)
  {
    delay(halfpress_ms);
  }

  digitalWrite(fullPressPin, HIGH);
  TCCR1A = ( (1 << COM1A0));

  // Wait a second
  delay(1000);

  TCCR1A = 0;
  digitalWrite(halfPressPin, LOW);
  digitalWrite(fullPressPin, LOW);
}
  
void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    char b = Serial.read();
    Serial.write("got ");
    Serial.println(b);

    if (b == '0')
    {
      Serial.println("immediate full shutter");
      runTestCycle(0);
    }
    else if (b == '1')
    {
      Serial.println("100ms half press before shutter");
      runTestCycle(100);
    }
    else if (b == '2')
    {
      Serial.println("1000ms half press before shutter");
      runTestCycle(1000);
    }
    else
    {
      Serial.println("Unknown command");
    }
    Serial.println("0-2?");
  }
}
