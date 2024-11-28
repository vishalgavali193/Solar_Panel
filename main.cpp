#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>
#include <Servo.h>

// Initialize the LCD with address 0x27, 16 columns, and 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize the INA219
Adafruit_INA219 ina219;

// Pin Definitions
#define SERVO1_PIN 3    // Pin connected to the film rotation servo
#define MOTOR_PIN 2     // Pin connected to the cleaning motor (via transistor)
#define PUMP_PIN 5      // Relay module control pin

const int ldrPin1 = A1; // LDR connected to analog pin A1
const int ldrPin2 = A3; // LDR connected to analog pin A3

// Timing constants
const unsigned long TOTAL_CYCLE_TIME = 120000; // 2 minutes in milliseconds
const unsigned long PUMP_DURATION = 10000;     // 1 minute in milliseconds
const unsigned long CLEANING_DURATION = 20000; // 2 minutes in milliseconds

unsigned long previousMillis = 0;
unsigned long pumpStartMillis = 0;
unsigned long cleaningStartMillis = 0;
bool pumpState = LOW;
bool cleaningState = LOW;
int currentAngle = 90; // Start at a neutral position

// Create Servo objects
Servo servo1; // Film rotation servo

void setup() {
  Wire.begin();  // Initialize I2C bus
  lcd.begin(16, 2);  // Initialize LCD with 16 columns and 2 rows
  lcd.backlight();
  
  if (!ina219.begin()) {
    lcd.clear();
    lcd.print("INA219 not found");
    while (1); // Halt if INA219 is not found
  }
  
  lcd.clear();
  lcd.print("Initializing...");

  // Attach the film rotation servo to its pin
  servo1.attach(SERVO1_PIN);
  
  pinMode(MOTOR_PIN, OUTPUT); // Set motor pin as output
  pinMode(PUMP_PIN, OUTPUT);  // Set pump pin as output
  digitalWrite(PUMP_PIN, LOW); // Ensure the pump is off initially
  
  Serial.begin(9600); // Start serial communication for debugging
}

void loop() {
  unsigned long currentMillis = millis();

  // Handle the pump and cleaning motor cycle
  if (currentMillis - previousMillis >= TOTAL_CYCLE_TIME) {
    previousMillis = currentMillis;
    
    // Start the pump
    pumpState = HIGH;
    digitalWrite(PUMP_PIN, pumpState);
    pumpStartMillis = currentMillis; // Start pump timer
    
    // Start the cleaning motor
    cleaningState = HIGH;
    cleaningStartMillis = currentMillis; // Start cleaning motor timer
  }
  
  // Handle pump duration
  if (pumpState && (currentMillis - pumpStartMillis >= PUMP_DURATION)) {
    pumpState = LOW;
    digitalWrite(PUMP_PIN, pumpState); // Turn off the pump
  }
  
  // Handle cleaning motor duration
  if (cleaningState && (currentMillis - cleaningStartMillis < CLEANING_DURATION)) {
    analogWrite(MOTOR_PIN, 255);  // Rotate cleaning motor at full speed
  } else if (cleaningState && (currentMillis - cleaningStartMillis >= CLEANING_DURATION)) {
    analogWrite(MOTOR_PIN, 0);    // Stop the cleaning motor after duration
    cleaningState = LOW;          // End cleaning state
  }

  // Adjust the angle of film rotation if cleaning is done
  if (!cleaningState) {
    // Read LDR values
    int ldrValue1 = analogRead(ldrPin1);
    int ldrValue2 = analogRead(ldrPin2);

    // Calculate the difference between the LDR values
    int ldrDifference = ldrValue1 - ldrValue2;

    // Adjust the current angle based on the LDR difference
    if (ldrDifference > 50) { // Threshold to prevent jitter
      currentAngle = min(currentAngle + 1, 180); // Increment angle, cap at 180
      delay(50); // Add delay to smooth out movement
    } else if (ldrDifference < -50) {
      currentAngle = max(currentAngle - 1, 0); // Decrement angle, cap at 0
      delay(50); // Add delay to smooth out movement
    }
    
    // Update the film rotation servo position
    servo1.write(currentAngle);
  }

  // Read voltage and current from INA219
  float voltage = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA(); // Current in milliamps

  // Display voltage and current on LCD
  lcd.setCursor(0, 0);
  lcd.print("Volt: ");
  lcd.print(voltage, 2); // Print voltage with 2 decimal places
  lcd.print(" V");

  lcd.setCursor(0, 1);
  lcd.print("I: ");
  lcd.print(current, 2); // Print current with 2 decimal places
  lcd.print(" mA");

  // Debugging output to the serial monitor
  Serial.print("LDR1: ");
  Serial.print(analogRead(ldrPin1));
  Serial.print(" | LDR2: ");
  Serial.print(analogRead(ldrPin2));
  Serial.print(" | Current Angle: ");
  Serial.println(currentAngle);

  // Wait for a short time before updating
  delay(500); // Update every 0.5 seconds
}
