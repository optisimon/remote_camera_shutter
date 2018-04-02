/*
 * Remote shutter controller - Panasonic DMC-G80, FZ20, and probably others
 *
 * Emulates a DMW-RS1
 *
 * WARNING: DO NOT USE ANYTHING IN THIS CODE, OR MAKE ANY ELECTRICAL CONNECTIONS
 *          TO ANY CAMERA UNLESS YOU KNOW EXACTLY WHAT YOU ARE DOING. EXPECT
 *          THAT THIS CODE AND RELATED INSTRUCTIONS ARE FLAWED AND WILL BRICK
 *          YOUR CAMERA.
 *
 *
 * Used circuit:
 *
 *                  _____             _____
 * 2.5mm pin4   ___|2k0  |____.______|2k7  |________________.__________.
 *                 |_____|    |      |_____|                |         _|_
 *                  _____     |                   _____     |        |36k|
 * arduino pin12___|330R |___|/  arduino pin11___|330R |___|/        |   |
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
 * NOTE: You can measure the shutter delay when the camera thinks a non-TTL
 *       flash is connected by connecting pin10 through a 330 Ohm resistor
 *       to the cameras center pin on the hot shoe. You don't need a ground
 *       connection for the DMC-G80, since the sides of the hot shoe has
 *       very low resistance to the ground in the remote shutter connection.
 *         Keep in mind that just because I can make that connection on my
 *       DMC-G80, this is not an assumption you can rely on, and previous or
 *       future batches of this camera might be bricked if that connection is
 *       done.
 *         Unfortunately, the shutter delay seems to be significantly larger
 *       when a flash is attached, so those delays are completely different
 *       than the delays when no flash is attached.
 *
 * NOTE: Uses pins 2 (lsb) to pin 9 (msb) as digital outputs to display duration
 *       since full shutter press as Gray-code. Has been used to drive LED:s,
 *       which have been photographed by the camera controlled by this remote
 *       shutter, to characterize its shutter lag.
 *
 *
 * Copyright (c) 2018 Simon Gustafsson (www.optisimon.com)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

const int halfPressPin = 11;
const int fullPressPin = 12;

const int numLeds = 8;
const unsigned char ledPins[] = {2, 3, 4, 5,  6, 7, 8, 9};

const int hotShoePin = 10;

unsigned int binaryToGray(unsigned int num)
{
    return num ^ (num >> 1);
}

void setup()
{
  pinMode(hotShoePin, INPUT);
  digitalWrite(hotShoePin, HIGH);
  
  digitalWrite(halfPressPin, LOW);
  digitalWrite(fullPressPin, LOW);
  pinMode(halfPressPin, OUTPUT);
  pinMode(fullPressPin, OUTPUT);

  for (int n = 0; n < numLeds; n++)
  {
    digitalWrite(ledPins[n], LOW);
    pinMode(ledPins[n], OUTPUT);
  }

  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Only needed for native USB port.
  }
  Serial.println("Connected:");
}

static short lastFlashDelay = -1;

void runTestCycle(int halfpress_ms)
{
  lastFlashDelay = -1;
  for (int n = 0; n < numLeds; n++)
  {
    digitalWrite(ledPins[n], LOW);
  }
  
  unsigned long previousMillis = millis();
  while (previousMillis == millis()) { /* just wait for ms transition */ };
  unsigned long startMillis = millis();
  
  digitalWrite(halfPressPin, HIGH);

  // Essentially: delay(halfpress_ms);
  while (millis() - startMillis < halfpress_ms) { /* just wait */ };
    
  digitalWrite(fullPressPin, HIGH);

  startMillis = millis();
  do {
    unsigned short currentDelay = millis() - startMillis;
    unsigned short grayCoded = binaryToGray(min(currentDelay, 255));

    // Set gray coded leds
    for (int n = 0; n < numLeds; n++)
    {
      digitalWrite(ledPins[n], (grayCoded & 1) ? HIGH : LOW);
      grayCoded = grayCoded >> 1;
    }

    if (digitalRead(hotShoePin) ==  LOW && lastFlashDelay == -1)
    {
      lastFlashDelay = currentDelay;
    }
    
  } while (millis() - startMillis < 1000);

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
    else if (b == '3')
    {
      Serial.println("100 cycles with 1000ms half press before shutter, and 2000 ms between cycles");
      for (int n = 0; n < 100; n++)
      {
        runTestCycle(1000);
        Serial.print("cycle ");
        Serial.print(n + 1);
        Serial.print(" of 100 :");
        if (lastFlashDelay == -1)
        {
          Serial.println(" camera did not use external flash");
        }
        else
        {
          Serial.print(" flash occured after ");
          Serial.print(lastFlashDelay);
          Serial.println(" ms");
        }
        delay(2000);
      }
    }
    else
    {
      Serial.println("Unknown command");
    }
    Serial.println("0-3?");
  }
}
