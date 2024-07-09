#include <Arduino.h>
#include <HT1621.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Wi-Fi credentials
#define WIFI_SSID "123"
#define WIFI_PASSWORD "L04GDLE3DBJ"

// Firebase project credentials
#define API_KEY "AIzaSyCvCa-sVylBjWPu7mvzdUvmIYtgQVX6raE"
#define DATABASE_URL "https://irrigation-farm-default-rtdb.europe-west1.firebasedatabase.app/" // Correct Firebase RTDB URL

// Flow sensor parameters
volatile int flowPulseCount1 = 0;
volatile int flowPulseCount2 = 0;

unsigned long oldTime1;
unsigned long oldTime2;
unsigned long lastPrintTime = 0;
unsigned long sendDataPrevMillis = 0;

float calibrationFactor = 4.5; // Calibration factor for the specific flow sensor
float volume1 = 0.0;
float volume2 = 0.0;

// Ball valve pins
const int valve1Pin = 25;
const int valve2Pin = 26;

// Push button pins
const int openButton1Pin = 6;
const int closeButton1Pin = 7;
const int openButton2Pin = 8;
const int closeButton2Pin = 9;

// HT1621 pins
const int ht1621DataPin = 32;
const int ht1621WritePin = 14;
const int ht1621ChipSelectPin = 12;

HT1621 lcd;
//float volume3 = 33;
// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

void pulseCounter1() {
  flowPulseCount1++;
}

void pulseCounter2() {
  flowPulseCount2++;
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set up the pins for the flow sensors
  pinMode(4, INPUT);
  pinMode(5, INPUT);

  // Attach interrupts to the flow sensors
  attachInterrupt(digitalPinToInterrupt(4), pulseCounter1, RISING);
  attachInterrupt(digitalPinToInterrupt(5), pulseCounter2, RISING);

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

  // WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Sign Up Success");
    signupOK = true;
  } else {
    Serial.printf("Firebase Sign Up Failed: %s\n", config.signer.signupError.message.c_str());
  }
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
  
  /* Send data to Firebase every 15 seconds
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setFloat(&fbdo, "sensor/volume1", volume1)) {
      Serial.println("Firebase Data Sent Successfully");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("Failed to Send Data to Firebase");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    }

   // Send data to Firebase every 15 seconds
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.setFloat(&fbdo, "sensor/volume1", volume3)) {
      Serial.println("Firebase Data Sent Successfully");
      Serial.print("PATH: ");
      Serial.println(fbdo.dataPath());
      Serial.print("TYPE: ");
      Serial.println(fbdo.dataType());
    } else {
      Serial.println("Failed to Send Data to Firebase");
      Serial.print("REASON: ");
      Serial.println(fbdo.errorReason());
    }
  

  } */

    // Send data to Firebase every 15 seconds
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    bool success = true;

    if (!Firebase.RTDB.setFloat(&fbdo, "sensor/volume1", volume1)) {
      Serial.println("Failed to Send Volume1 Data to Firebase");
      Serial.print("REASON: ");
      Serial.println(fbdo.errorReason());
      success = false;
    }

    if (!Firebase.RTDB.setFloat(&fbdo, "sensor/volume2", volume2)) {
      Serial.println("Failed to Send Volume2 Data to Firebase");
      Serial.print("REASON: ");
      Serial.println(fbdo.errorReason());
      success = false;
    }

    if (success) {
      Serial.println("Firebase Data Sent Successfully");
      Serial.print("Volume1 PATH: ");
      Serial.println(fbdo.dataPath());
      Serial.print("Volume2 PATH: ");
      Serial.println(fbdo.dataPath());
      Serial.print("TYPE: ");
      Serial.println(fbdo.dataType());
    }
  }
}