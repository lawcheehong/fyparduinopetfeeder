#include <Servo.h>              // Include the Servo Motor library
#include <Wire.h>               // Include the I2C library
#include <LiquidCrystal_I2C.h>  // Include the I2C LCD library
#include <RTClib.h>             // Include the RTC library
#include <SoftwareSerial.h>     // Include the SoftwareSerial library for Bluetooth
#include <EEPROM.h>             //Include the EEPROM library

// Define the servo motor, button, and buzzer pins
const int servoPin = 9;
const int buttonPin = 7;
const int buzzerPin = 3;

// Initialize the I2C LCD display with its I2C address
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Change the address if different

// Initialize the RTC object
RTC_DS1307 rtc;

Servo servoMotor;
int buttonState = 0;
bool isRunning = false;

// Set Bluetooth Timer
int seq = 0;
String hourInStr = "";
String minInStr = "";
String secInStr = "";

int hour1 = -1;
int min1 = -1;
int sec1 = -1;

int hour2 = -1;
int min2 = -1;
int sec2 = -1;

bool timer1 = false;
bool timer2 = false;

// EEPROM
int address1 = 0;   // Base on hour1
int address2 = 10;  // Base on hour2

// Setup the UNO
void setup() {
  // EEPROM Total address base on the board, for UNO is 1024, so can't more than 1023
  EEPROM.begin();
  // Start the I2C communication for the LCD display and RTC
  lcd.backlight();
  Wire.begin();
  lcd.begin(16, 2);  // Set the LCD size (columns, rows)
  Serial.begin(9600);

  // Attach the servo to its pin
  servoMotor.attach(servoPin);

  // Set the button and buzzer pins as input and output respectively
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  // Initialize the RTC
  if (!rtc.begin()) {
    lcd.print("RTC Failed");
    while (1)
      ;
  }

  // Uncomment this line to set the RTC to the current date and time (only once)
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Print a message on the LCD to indicate successful initialization
  lcd.print("RTC Initialized");
  delay(1500);
  lcd.clear();

  while (!Serial) {
    // wait for serial port toÂ connect
  }
}

void loop() {
  // Address to value
  int value1 = EEPROM.read(address1);
  int value2 = EEPROM.read(address1 + 1);
  int value3 = EEPROM.read(address1 + 2);
  // Serial.print(value1);  // Testing unplug power for hour 1
  // Serial.print(value2);
  // Serial.println(value3);

  int value4 = EEPROM.read(address2);
  int value5 = EEPROM.read(address2 + 1);
  int value6 = EEPROM.read(address2 + 2);
  // Serial.print(value4);  // Testing unplug power for hour 2
  // Serial.print(value5);
  // Serial.println(value6);

  // Read the button state
  buttonState = digitalRead(buttonPin);

  // Get the current time from RTC
  DateTime now = rtc.now();

  // Display date on the first row
  lcd.setCursor(0, 0);
  lcd.print("Date: " + now.timestamp(DateTime::TIMESTAMP_DATE));

  // Display time on the second row
  lcd.setCursor(0, 1);
  lcd.print("Time: " + now.timestamp(DateTime::TIMESTAMP_TIME));

  // Check if the button is pressed
  if (buttonState == HIGH) {
    runBuzzer();  // Call the function to run the buzzer
    runServo();   // Call the function to run the servo
  } else {
    stopBuzzer();  // Call the function to stop the buzzer
  }

  // Check if the current time matches timer 1
  if (now.hour() == value1 && now.minute() == value2 && now.second() == value3) {
    runBuzzer();  // Call the function to run the buzzer
    runServo();   // Call the function to run the servo
  }

  // Check if the current time matches timer 2
  if (now.hour() == value4 && now.minute() == value5 && now.second() == value6) {
    runBuzzer();  // Call the function to run the buzzer
    runServo();   // Call the function to run the servo
  }

  // Add a small delay to prevent rapid button detection
  delay(100);

  // Check for Bluetooth commands
  while (Serial.available() > 0) {
    int inputChar = Serial.read();
    // Serial.print(inputChar); // Checking use
    if (inputChar == 65 || inputChar == 97) {
      timer1 = true;
      timer2 = false;
    } else if (inputChar == 66 || inputChar == 98) {
      timer1 = false;
      timer2 = true;
    }

    if (isDigit(inputChar)) {
      // Serial.print(isDigit(inputChar)); // Checking use
      if (seq == 0) {
        hourInStr += (char)inputChar;
      } else if (seq == 1) {
        minInStr += (char)inputChar;
      } else {
        secInStr += (char)inputChar;
      }

    } else if (inputChar == 58) {  // 58 = ":"
      // if meet ":" sign, then seq increment
      seq++;
    } else if (inputChar == 10) {  // 10 = "space"
      // covert string to int
      if (timer1) {
        hour1 = hourInStr.toInt();
        min1 = minInStr.toInt();
        sec1 = secInStr.toInt();
        for (int i = address1; i < EEPROM.length(); i++) {
          EEPROM.write(i, 0);
        }
        Serial.println("Writing hour1 to EEPROM: " + hour1);
        EEPROM.write(address1, hour1);
        EEPROM.write(address1 + 1, min1);
        EEPROM.write(address1 + 2, sec1);
      } else {
        hour2 = hourInStr.toInt();
        min2 = minInStr.toInt();
        sec2 = secInStr.toInt();
        for (int i = address2; i < EEPROM.length(); i++) {
          EEPROM.write(i, 0);
        }
        Serial.println("Writing hour2 to EEPROM: " + hour2);
        EEPROM.write(address2, hour2);
        EEPROM.write(address2 + 1, min2);
        EEPROM.write(address2 + 2, sec2);
      }

      // Check
      Serial.println("HOUR1: ");
      Serial.print(hour1);
      Serial.print(min1);
      Serial.println(sec1);
      Serial.println("HOUR2: ");
      Serial.print(hour2);
      Serial.print(min2);
      Serial.println(sec2);

      // Reset Everything (For clear the timer, uncomment when need)
      seq = 0;
      hourInStr = "";
      minInStr = "";
      secInStr = "";
      break;
    }
  }
}

void runServo() {
  // Set the flag to indicate the action is running
  isRunning = true;

  // Move the servo to angle 90
  servoMotor.write(90);

  // Wait for 3 seconds
  delay(3000);

  // Move the servo back to angle 0
  servoMotor.write(0);

  // Reset the flag
  isRunning = false;
}

void runBuzzer() {
  // Beep the buzzer
  digitalWrite(buzzerPin, LOW);
}

void stopBuzzer() {
  // Stop the buzzer
  digitalWrite(buzzerPin, HIGH);
}
