/* Animatronics workshop sketch
   Uses two analogue joysticks and four servos.

   First joystick moves servo1 & servo 2,
   button start/stop recording positions.

   Second joystick moves servo3 & servo 4,
   button start/stop playing recorded positions.
   Press button for 2 seconds to autoplay.

   Pins:
   Arduino    Stick1    Stick2    Servo1   Servo2  Servo3    Servo4   Record/
      GND       GND       GND    Brown     Brown   Brown     Brown    Auto play
       5V       VCC       VCC      Red       Red     Red       Red    LED
       A0       HOR
       A1       VER
       PD2      BUTT
       A2                 HOR
       A3                 VER
       PD3                BUTT
       11                       Yellow
       10                                 Yellow
        9                                         Yellow
        6                                                   Yellow
       PD4                                                            X
*/
#include <Servo.h>

bool repeatePlaying = false; /* Repeatedly is running recorded cycle */
int delayBetweenCycles = 2000; /* Delay between cycles */

int servo1Pin = 11;       /* Servo1 servo */
int servo2Pin = 10;   /* Servo2 servo */
int servo3Pin = 9;       /* Servo3 servo */
int servo4Pin = 6;     /* Servo4 servo */

int xdirPin = 0;        /* Servo1 - joystick1*/
int ydirPin = 1;        /* Servo2 - joystick1 */
int zdirPin = 3;        /* Servo3 - joystick2 */
int gdirPin = 2;        /* Servo4 - joystick2 */

//int pinRecord = A4;     /* Button record - backward compatibility */
//int pinPlay = A5;       /* Button play  - backward compatibility */
int pinRecord = PD2;     /* Button record - recommended (A4 is deprecated, will by used for additional joystick) */
int pinPlay = PD3;       /* Button play  - recommended (A5 is deprecated, will by used for additional joystick) */
int pinLedRecord = 13;  /* LED - indicates recording (light) or auto play mode (blink one) */

bool useInternalPullUpResistors = true;

const int buffSize = 512; /* Size of recording buffer */

int startServo1 = 90;
int startServo2 = 90;
int startServo3 = 90;
int startServo4 = 90;

int posServo1 = 90;
int posServo2 = 90;
int posServo3 = 90;
int posServo4 = 90;

int lastServo1 = 90;
int lastServo2 = 90;
int lastServo3 = 90;
int lastServo4 = 90;

int minServo1 = 0;
int maxServo1 = 150;
int minServo2 = 0;
int maxServo2 = 150;
int minServo3 = 0;
int maxServo3 = 150;
int minServo4 = 0;
int maxServo4 = 150;

const int countServo = 4;
int buff[buffSize];
int buffAdd[countServo];
int recPos = 0;
int playPos = 0;

int buttonRecord = HIGH;
int buttonPlay = HIGH;

int buttonRecordLast = LOW;
int buttonPlayLast = LOW;

bool record = false;
bool play = false;
bool debug = false;

String command = "Manual";
int printPos = 0;

int buttonPlayDelay = 20;
int buttonPlayCount = 0;

bool ledLight = false;

Servo servoServo1;
Servo servoServo2;
Servo servoServo3;
Servo servoServo4;

void setup() {
  Serial.begin(9600);

  if (useInternalPullUpResistors) {
    pinMode(pinRecord, INPUT_PULLUP);
    pinMode(pinPlay, INPUT_PULLUP);
  }
  else
  {
    pinMode(pinRecord, INPUT);
    pinMode(pinPlay, INPUT);
  }

  pinMode(xdirPin, INPUT);
  pinMode(ydirPin, INPUT);
  pinMode(zdirPin, INPUT);
  pinMode(gdirPin, INPUT);

  pinMode(pinLedRecord, OUTPUT);

  servoServo1.attach(servo1Pin);
  servoServo2.attach(servo2Pin);
  servoServo3.attach(servo3Pin);
  servoServo4.attach(servo4Pin);

  StartPosition();

  digitalWrite(pinLedRecord, HIGH);
  delay(1000);
  digitalWrite(pinLedRecord, LOW);
}

void loop() {

  buttonRecord = digitalRead(pinRecord);
  buttonPlay = digitalRead(pinPlay);

  //  Serial.print(buttonRecord);
  //  Serial.print("\t");
  //  Serial.println(buttonPlay);
  //  for testing purposes

  if (buttonPlay == LOW)
  {
    buttonPlayCount++;

    if (buttonPlayCount >= buttonPlayDelay)
    {
      repeatePlaying = true;
    }
  }
  else buttonPlayCount = 0;

  if (buttonPlay != buttonPlayLast)
  {
    if (record)
    {
      record = false;
    }

    if (buttonPlay == LOW)
    {
      play = !play;
      repeatePlaying = false;

      if (play)
      {
        StartPosition();
      }
    }
  }

  if (buttonRecord != buttonRecordLast)
  {
    if (buttonRecord == LOW)
    {
      record = !record;

      if (record)
      {
        play = false;
        repeatePlaying = false;
        recPos = 0;
      }
      else
      {
        if (debug) PrintBuffer();
      }
    }
  }

  buttonPlayLast = buttonPlay;
  buttonRecordLast = buttonRecord;

  float dx = map(analogRead(xdirPin), 0, 1023, -5.0, 5.0);
  float dy = map(analogRead(ydirPin), 0, 1023, 5.0, -5.0);
  float dz = map(analogRead(zdirPin), 0, 1023, 5.0, -5.0);
  float dg = map(analogRead(gdirPin), 0, 1023, 5.0, -5.0);

  if (abs(dx) < 1.5) dx = 0;
  if (abs(dy) < 1.5) dy = 0;
  if (abs(dz) < 1.5) dz = 0;
  if (abs(dg) < 1.5) dg = 0;

  posServo1 += dx;
  posServo2 += dy;
  posServo3 += dz;
  posServo4 += dg;

  if (play)
  {
    if (playPos >= recPos) {
      playPos = 0;

      if (repeatePlaying)
      {
        delay(delayBetweenCycles);
        StartPosition();
      }
      else
      {
        play = false;
      }
    }

    bool endOfData = false;

    while (!endOfData)
    {
      if (playPos >= buffSize - 1) break;
      if (playPos >= recPos) break;

      int data = buff[playPos];
      int angle = data & 0xFFF;
      int servoNumber = data & 0x3000;
      endOfData = data & 0x4000;

      switch (servoNumber)
      {
        case 0x0000:
          posServo1 = angle;
          break;

        case 0x1000:
          posServo2 = angle;
          break;

        case 0x2000:
          posServo3 = angle;
          break;

        case 0x3000:
          posServo4 = angle;
          dg = posServo4 - lastServo4;
          break;
      }

      playPos++;
    }
  }

  if (posServo1 > maxServo1) posServo1 = maxServo1;
  if (posServo2 > maxServo2) posServo2 = maxServo2;
  if (posServo3 > maxServo3) posServo3 = maxServo3;
  if (posServo4 > maxServo4) posServo4 = maxServo4;

  if (posServo1 < minServo1) posServo1 = minServo1;
  if (posServo2 < minServo2) posServo2 = minServo2;
  if (posServo3 < minServo3) posServo3 = minServo3;
  if (posServo4 < minServo4) posServo4 = minServo4;

  servoServo1.write(posServo1);
  servoServo2.write(posServo2);
  servoServo3.write(posServo3);

  bool waitServo4 = false;
  if (dg < 0) {
    posServo4 = minServo4;
    waitServo4 = true;
  }
  else if (dg > 0) {
    posServo4 = maxServo4;
    waitServo4 = true;
  }

  servoServo4.write(posServo4);
  if (play && waitServo4)
  {
    delay(1000);
  }

  if ((lastServo1 != posServo1) | (lastServo2 != posServo2) | (lastServo3 != posServo3) | (lastServo4 != posServo4))
  {
    if (record)
    {
      if (recPos < buffSize - countServo)
      {
        int buffPos = 0;

        if (lastServo1 != posServo1)
        {
          buffAdd[buffPos] = posServo1;
          buffPos++;
        }

        if (lastServo2 != posServo2)
        {
          buffAdd[buffPos] = posServo2 | 0x1000;
          buffPos++;
        }

        if (lastServo3 != posServo3)
        {
          buffAdd[buffPos] = posServo3 | 0x2000;
          buffPos++;
        }

        if (lastServo4 != posServo4)
        {
          buffAdd[buffPos] = posServo4 | 0x3000;
          buffPos++;
        }

        buffAdd[buffPos - 1] = buffAdd[buffPos - 1] | 0x4000;

        for (int i = 0; i < buffPos; i++)
        {
          buff[recPos + i] = buffAdd[i];
        }

        recPos += buffPos;
      }
    }

    command = "Manual";
    printPos = 0;

    if (play)
    {
      command = "Play";
      printPos = playPos;
    }
    else if (record)
    {
      command = "Record";
      printPos = recPos;
    }

    Serial.print(command);
    Serial.print("\t");
    Serial.print(printPos);
    Serial.print("\t");
    Serial.print(posServo1);
    Serial.print("\t");
    Serial.print(posServo2);
    Serial.print("\t");
    Serial.print(posServo3);
    Serial.print("\t");
    Serial.print(posServo4);
    Serial.print("\t");
    Serial.print(record);
    Serial.print("\t");
    Serial.print(play);
    Serial.println();
  }

  lastServo1 = posServo1;
  lastServo2 = posServo2;
  lastServo3 = posServo3;
  lastServo4 = posServo4;

  if ( repeatePlaying)
  {
    ledLight = !ledLight;
  }
  else
  {
    if (ledLight)
    {
      ledLight = false;
    }

    if (record)
    {
      ledLight = true;
    }
  };

  digitalWrite(pinLedRecord, ledLight);
  delay(50);
}

void PrintBuffer()
{
  for (int i = 0; i < recPos; i++)
  {
    int data = buff[i];
    int angle = data & 0xFFF;
    int servoNumber = data & 0x3000;
    bool endOfData = data & 0x4000;

    Serial.print("Servo=");
    Serial.print(servoNumber);
    Serial.print("\tAngle=");
    Serial.print(angle);
    Serial.print("\tEnd=");
    Serial.print(endOfData);
    Serial.print("\tData=");
    Serial.print(data, BIN);
    Serial.println();
  }
}

void StartPosition()
{
  int angleServo1 = servoServo1.read();
  int angleServo2 = servoServo2.read();
  int angleServo3 = servoServo3.read();
  int angleServo4 = servoServo4.read();

  Serial.print(angleServo1);
  Serial.print("\t");
  Serial.print(angleServo2);
  Serial.print("\t");
  Serial.print(angleServo3);
  Serial.print("\t");
  Serial.print(angleServo4);
  Serial.println("\t");

  posServo1 = startServo1;
  posServo2 = startServo2;
  posServo3 = startServo3;
  posServo4 = startServo4;

  servoServo1.write(posServo1);
  servoServo2.write(posServo2);
  servoServo3.write(posServo3);
  servoServo4.write(posServo4);
}
