#include <Arduino.h>
#include <HT1621.h>

// Flow sensor parameters
volatile int flowPulseCount1 = 0;
volatile int flowPulseCount2 = 0;

unsigned long oldTime1;
unsigned long oldTime2;
unsigned long lastPrintTime = 0;

float calibrationFactor = 4.5; // Calibration factor for the specific flow sensor
float volume1 = 0.0;
float volume2 = 0.0;

// Ball valve pins
const int valve1Pin = 4;
const int valve2Pin = 5;

// Push button pins
const int openButton1Pin = 6;
const int closeButton1Pin = 7;
const int openButton2Pin = 8;
const int closeButton2Pin = 9;

// HT1621 pins
const int ht1621DataPin = 10;
const int ht1621WritePin = 11;
const int ht1621ChipSelectPin = 12;

HT1621 lcd;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Set up the pins for the flow sensors
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  // Attach interrupts to the flow sensors
  attachInterrupt(digitalPinToInterrupt(2), pulseCounter1, RISING);
  attachInterrupt(digitalPinToInterrupt(3), pulseCounter2, RISING);

  // Set up the pins for the ball valves
  pinMode(valve1Pin, OUTPUT);
  pinMode(valve2Pin, OUTPUT);
  digitalWrite(valve1Pin, LOW); // Close valve initially
  digitalWrite(valve2Pin, LOW); // Close valve initially

  // Set up the pins for the push buttons
  pinMode(openButton1Pin, INPUT_PULLUP);
  pinMode(closeButton1Pin, INPUT_PULLUP);
  pinMode(openButton2Pin, INPUT_PULLUP);
  pinMode(closeButton2Pin, INPUT_PULLUP);

  // Initialize the HT1621 LCD screen
  lcd.begin(ht1621ChipSelectPin, ht1621WritePin, ht1621DataPin);
  lcd.clear();
  
  // Initialize timing
  oldTime1 = millis();
  oldTime2 = millis();
}

void loop() {
  // Handle button presses to control the ball valves
  if (digitalRead(openButton1Pin) == LOW) {
    digitalWrite(valve1Pin, HIGH); // Open valve 1
  }
  if (digitalRead(closeButton1Pin) == LOW) {
    digitalWrite(valve1Pin, LOW); // Close valve 1
  }
  if (digitalRead(openButton2Pin) == LOW) {
    digitalWrite(valve2Pin, HIGH); // Open valve 2
  }
  if (digitalRead(closeButton2Pin) == LOW) {
    digitalWrite(valve2Pin, LOW); // Close valve 2
  }

  // Calculate volume for sensor 1
  if (millis() - oldTime1 > 1000) { // Update every second
    detachInterrupt(digitalPinToInterrupt(2)); // Disable interrupt temporarily
    
    float flowRate1 = ((1000.0 / (millis() - oldTime1)) * flowPulseCount1) / calibrationFactor;
    volume1 += (flowRate1 / 60.0) * 1000.0; // Convert flow rate to volume (L/min to ml)
    oldTime1 = millis();
    flowPulseCount1 = 0; // Reset pulse count for next interval
    
    attachInterrupt(digitalPinToInterrupt(2), pulseCounter1, RISING); // Re-enable interrupt
  }

  // Calculate volume for sensor 2
  if (millis() - oldTime2 > 1000) { // Update every second
    detachInterrupt(digitalPinToInterrupt(3)); // Disable interrupt temporarily
    
    float flowRate2 = ((1000.0 / (millis() - oldTime2)) * flowPulseCount2) / calibrationFactor;
    volume2 += (flowRate2 / 60.0) * 1000.0; // Convert flow rate to volume (L/min to ml)
    oldTime2 = millis();
    flowPulseCount2 = 0; // Reset pulse count for next interval
    
    attachInterrupt(digitalPinToInterrupt(3), pulseCounter2, RISING); // Re-enable interrupt
  }

  // Print the volumes every 2 seconds
  if (millis() - lastPrintTime >= 2000) { // Print once every 2 seconds
    Serial.print("Volume from sensor 1: ");
    Serial.print(volume1);
    Serial.println(" ml");
    
    Serial.print("Volume from sensor 2: ");
    Serial.print(volume2);
    Serial.println(" ml");

    // Update the LCD with volume from sensor 1
    lcd.clear();
    lcd.print("V1:");
    lcd.print(volume1, 1); // Print volume1 with 1 decimal place
    lcd.print("ml");

    lastPrintTime = millis();
  }
}

void pulseCounter1() {
  flowPulseCount1++;
}

void pulseCounter2() {
  flowPulseCount2++;
}