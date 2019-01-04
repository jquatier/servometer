/*
  Servometer - an analog meter display using Arduino and the Servo library
  Recieves data over serial connection from a NodeJS application pulling data from APIs
  @author Jacob Quatier
  
  Additional Libraries: Adafruit Industries for example code and libraries for controlling LED displays
  https://github.com/adafruit/Adafruit-LED-Backpacks
*/

#include <Wire.h>
#include <Servo.h> 
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

// initialize hardware libraries
Adafruit_AlphaNum4 display1 = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 display2 = Adafruit_AlphaNum4();
Adafruit_BicolorMatrix matrix = Adafruit_BicolorMatrix();
Servo servo;

static const uint8_t PROGMEM
  face_good[] =
  { B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10100101,
    B10011001,
    B01000010,
    B00111100 },
  face_ok[] =
  { B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10111101,
    B10000001,
    B01000010,
    B00111100 },
  face_bad[] =
  { B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10011001,
    B10100101,
    B01000010,
    B00111100 };

// constants
static const int SERVO_PIN = 9;
static const int SERVO_MAX = 180;
static const int SERVO_MIN = 0;
static const int MAX_NO_DATA = 60;
static const String NOT_STRING = " NOT";
static const String SURE_STRING = "SURE";

// initial values
String display1Value = NOT_STRING;
String display2Value = SURE_STRING;
String currentHealth = "";
int servoAngle = SERVO_MAX;
int currentServoAngle = SERVO_MAX;
int noDataCount = 0;

/*
  Setup, initialize i2c and other pins
*/
void setup() 
{
  Serial.begin(9600);
  
  // initialize hardware pins
  display1.begin(0x70);
  display2.begin(0x72);
  matrix.begin(0x74);
  servo.attach(SERVO_PIN);
  
  cycleServo();
}

/*
  Main loop. Wait for data over serial USB and display when data is available
*/
void loop() 
{
  // read in metrics over serial (USB)
  String receivedData = "";
  if(Serial.available() > 0)
  {
    receivedData = Serial.readStringUntil('\n');
  }

  // wait for data
  if (receivedData != "") 
  {
    // send what was recieved as confirmation back to host app
    Serial.println("received: " + receivedData);
    display1Value = formatValueForSegmentDisplay(getValueFromResponse(receivedData, ':', 0));
    display2Value = formatValueForSegmentDisplay(getValueFromResponse(receivedData, ':', 1));
    currentHealth = getValueFromResponse(receivedData, ':', 2);
    servoAngle = findServoAngle(display1Value);
    Serial.println("new servo angle: " + servoAngle);
    noDataCount = 0;
  } 
  else 
  {
    noDataCount++;
  }
  
  // if there hasn't been any data in awhile, reset the display so we know something's wrong.
  if(noDataCount >= MAX_NO_DATA) 
  {
    display1Value = NOT_STRING;
    display2Value = SURE_STRING;
    currentHealth = "";
    noDataCount = 0;
  }
 
  // write values to display1 buffer
  display1.writeDigitAscii(0, display1Value[0]);
  display1.writeDigitAscii(1, display1Value[1]);
  display1.writeDigitAscii(2, display1Value[2]);
  display1.writeDigitAscii(3, display1Value[3]);
  
  // write values to display2 buffer
  display2.writeDigitAscii(0, display2Value[0]);
  display2.writeDigitAscii(1, display2Value[1]);
  display2.writeDigitAscii(2, display2Value[2]);
  display2.writeDigitAscii(3, display2Value[3]);
 
  // write displays
  display1.writeDisplay();
  display2.writeDisplay();
  
  // update matrix for health display
  updateHealthMatrix(currentHealth);
  
  // move servo angle for meter
  moveServo();
  
  delay(500);
}

void moveServo() {
  if(servoAngle == currentServoAngle) {
    return;
  }
  else {
    while(currentServoAngle != servoAngle) {
      currentServoAngle += (servoAngle > currentServoAngle ? 1 : -1);
      servo.write(currentServoAngle);
      delay(50);
    }
  }
}

void updateHealthMatrix(String health)
{
  // write health to matrix buffer
  matrix.clear();
  if(health == "green") {
    matrix.drawBitmap(0, 0, face_good, 8, 8, LED_GREEN);
  } else if(health == "orange") {
    matrix.drawBitmap(0, 0, face_ok, 8, 8, LED_YELLOW);
  } else if(health == "red") {
    matrix.drawBitmap(0, 0, face_bad, 8, 8, LED_RED);
  } else {
    matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
    matrix.setTextSize(1);
    matrix.setTextColor(LED_GREEN);
    matrix.setCursor(2,0);
    matrix.print("?");
  }
  matrix.writeDisplay();
}

String getValueFromResponse(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/*
  Convert response time metric to usable servo angle
*/
int findServoAngle(String value)
{
  return constrain((180 - map(value.toInt(), 0, 2000, SERVO_MIN, SERVO_MAX)), SERVO_MIN, SERVO_MAX);
}

/*
  Adds padding for the segmented LED displays so that numbers show correctly.
*/
String formatValueForSegmentDisplay(String value) 
{
  while(value.length() < 4) 
  {
    value = ' ' + value;
  }
  return value;
}
  

/*
  Scan servo to it's min and max position for initialization 
  (mainly to validate the needle is correctly attached)
*/
void cycleServo() 
{
  // now scan back from 180 to 0 degrees
  for(int servoAngle = SERVO_MAX; servoAngle > SERVO_MIN; servoAngle--)    
  {                                
    servo.write(servoAngle);           
    delay(5);       
  } 
  // scan from 0 to 180 degrees
  for(int servoAngle = SERVO_MIN; servoAngle < SERVO_MAX; servoAngle++)  
  {                                  
    servo.write(servoAngle);               
    delay(5);                   
  } 
}
