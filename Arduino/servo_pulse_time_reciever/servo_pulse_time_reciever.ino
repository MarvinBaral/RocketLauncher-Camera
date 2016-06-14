//this is a firmware simply there to be able to control 3 Servos with serial communication from your PC
#include "sharedConstants.h"
const unsigned short int PULSE_LENGTH = 20000; //microseconds
const unsigned short int MIN_PULSE = 500;
const unsigned short int MAX_PULSE = 1810;
const unsigned short int REPEATIONS_OF_NEW_PULSE_TIME = 10;
const unsigned short int UPTIME_SHOOTING_POS = 1050;
const unsigned short int UPTIME_SHOOT_LOCK = 1200;
const unsigned short int INITIAL_PULSE_TIMES[3] = {1150, 1500, 1200};
const float TIME_PER_DEGREE = 10.69; //for both

unsigned short int pulseTimes[3];//in ms
int degree;
int servoPins[3] = {9, 10, 11}; //also usable as PWM
char select;
unsigned long int startMicros;
unsigned long int timeInPulse;
unsigned short int ctr = 0;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 3; i++) {
    pinMode(servoPins[i], OUTPUT);
    pulseTimes[i] = INITIAL_PULSE_TIMES[i];
  }
}


/*Pulses done by main loop (there are three of them in parallel):

pulseTimes[0]
|      /
|     |

+-----+                         
|     |                         
|     |                         
+     +--------------------------

      +-----+                      
      |     |                      
      |     |                      
------+     +--------------------

            +-----+                      
            |     |                      
            |     |                      
------------+     +--------------

|                               |
|                                \
time defined in PULSE_LENGTH (20ms)

|
|
time = 0

*/

void loop() {
  if (ctr < REPEATIONS_OF_NEW_PULSE_TIME) {
    startMicros = micros();
    for (int pin = 0; pin < 3; pin++) {
      digitalWrite(servoPins[pin], HIGH);
      delayMicroseconds(pulseTimes[pin]);
      digitalWrite(servoPins[pin], LOW);
    }
    do {
      timeInPulse = micros() - startMicros;
    }
    while (timeInPulse < PULSE_LENGTH);
    ctr++;
  }
}

//The protocoll
//=============
//<char select {0-1}>;<int pulseTime {MIN_PULSE-MAX_PULSE}>; or: a, s

//used in the same way as acknowledgement

void printErr() {
  Serial.print("Malformed expression. Right protocol usage: ");
  Serial.print("<char select {0-1}>;<int pulseTime {");
  Serial.print(MAX_DEGREES[0][0]);
  Serial.print(" to ");
  Serial.print(MAX_DEGREES[1][1]);
  Serial.print("}> or: s, a");  
}

void pulseServo(int pin, int uptime, int cycles) {
  for (int cycle = 0; cycle < cycles; cycle++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(uptime);
    digitalWrite(pin, LOW);
    delayMicroseconds(PULSE_LENGTH - uptime);
  }
}

void serialEvent(){
  if (Serial.available()) {
    select = Serial.read();
    if (select == 's') {
      pulseTimes[2] = UPTIME_SHOOTING_POS;
      pulseServo(servoPins[2], pulseTimes[2], 10);
      pulseTimes[2] = INITIAL_PULSE_TIMES[2];
      ctr = 0;
      Serial.println("shot");
    } else {
      select -= '0'; //convert char to number
      if (select == 0 || select == 1 && Serial.find(';')) {
        degree = Serial.parseInt();
        if (degree >= MAX_DEGREES[0][0] && degree <= MAX_DEGREES[1][1] && Serial.find(';')) {
          pulseTimes[select] = INITIAL_PULSE_TIMES[select] - degree * TIME_PER_DEGREE;
          ctr = 0;
          Serial.print(char(select + '0')); //!!! modified char
          Serial.print(';');
          Serial.print(pulseTimes[select]);
          Serial.println(';');
        } else {
          printErr();
        }
      } else {
        if (select == ('a' - '0')) { //!!! modified char
          for (int i = 0; i < 3; i++) {
            Serial.print(i);
            Serial.print(": ");
            Serial.println(pulseTimes[i]);
          }
        } else {
          printErr();
        }
      }
    }
  }
}

//micros() overflow after 70 min!!!

