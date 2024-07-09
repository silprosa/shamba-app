#include <LiquidCrystal_I2C.h>  // Include LCD library
#include <SoftwareSerial.h>     // Include software serial library
#include <FirebaseESP8266.h>    // Include Firebase library

#define FLOW_PIN1 2  // Digital pin connected to YF-S201 sensor 1 (pulse output)
#define FLOW_PIN2 3  // Digital pin connected to YF-S201 sensor 2

// Number of pulses per liter for each sensor (adjust based on your sensor datasheet)
const float CALIBRATION_FACTOR1 = 7.5;  // Pulses per liter for sensor 1
const float CALIBRATION_FACTOR2 = 7.5;  // Pulses per liter for sensor 2 (might differ slightly)

volatile unsigned long flowCount1 = 0;  // Count pulses for sensor 1
volatile unsigned long flowCount2 = 0;  // Count pulses for sensor 2

// Timing variables
unsigned long lastMillis = 0;
const int interval = 1000;        // Calculation interval in milliseconds (1 second)

float flowRate1 = 0.0;  // Flow rate (liters per minute) for sensor 1
float flowRate2 = 0.0;  // Flow rate (liters per minute) for sensor 1
float totalVolume1 = 0.0;  // Total volume (liters) for sensor 1
float totalVolume2 = 0.0;  // Total volume (liters) for sensor 2

// LCD communication (assuming a 16x2 character LCD with I2C address 0x27)
const int lcd_address = 0x27;
const int lcd_rows = 2;
const int lcd_cols = 16;
LiquidCrystal_I2C lcd(lcd_address, lcd_rows, lcd_cols);

// GSM module communication (SoftwareSerial on pins 7 and 8)
SoftwareSerial gsmSerial(7, 8);

// Define pins for actuators and buttons
#define ACTUATOR1_PIN 4
#define ACTUATOR2_PIN 5
#define BUTTON1_PIN 6
#define BUTTON2_PIN 9

// Firebase credentials and database URL
#define FIREBASE_HOST "your-project-id.firebaseio.com"
#define FIREBASE_AUTH "your-secret-auth-token"

// Firebase path to store data
#define FIREBASE_PATH "/flow_data"

// Firebase object
FirebaseData firebaseData;

void setup() {
  Serial.begin(9600);  // Start serial communication for debugging

  // Set sensor pins as input
  pinMode(FLOW_PIN1, INPUT_PULLUP);
  pinMode(FLOW_PIN2, INPUT_PULLUP);

  // Set actuator pins as output
  pinMode(ACTUATOR1_PIN, OUTPUT);
  pinMode(ACTUATOR2_PIN, OUTPUT);

  // Set button pins as input
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // Initialize actuators to off (assuming LOW is off and HIGH is on)
  digitalWrite(ACTUATOR1_PIN, LOW);
  digitalWrite(ACTUATOR2_PIN, LOW);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Vol. 1:   L");  // Initial display for volume 1
  lcd.setCursor(0, 1);
  lcd.print("Vol. 2:   L");  // Initial display for volume 2

  // Initialize GSM module
  gsmSerial.begin(9600);
  delay(1000);  // Give some time for the GSM module to initialize
  gsmSerial.println("AT");  // Check communication
  delay(100);
  gsmSerial.println("AT+CSQ");  // Check signal quality
  delay(100);

  // Connect to Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  // Print initial messages to Serial Monitor
  Serial.println("Flow Rate Monitor and Control System");
  Serial.println("====================================");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= interval) {
    // Calculate flow rates
    flowRate1 = (flowCount1 * 60.0 * 1000.0) / (CALIBRATION_FACTOR1 * interval);
    flowRate2 = (flowCount2 * 60.0 * 1000.0) / (CALIBRATION_FACTOR2 * interval);

    // Update total volumes
    totalVolume1 += flowRate1 * interval / 1000.0;
    totalVolume2 += flowRate2 * interval / 1000.0;

    // Reset flow counts for next interval
    flowCount1 = 0;
    flowCount2 = 0;

    // Update LCD display with total volumes (round to 2 decimal places)
    lcd.setCursor(0, 0);
    lcd.print("Vol. 1: ");
    lcd.print(totalVolume1, 2);  // Round to 2 decimal places
    lcd.print(" L");
    lcd.setCursor(0, 1);
    lcd.print("Vol. 2: ");
    lcd.print(totalVolume2, 2);
    lcd.print(" L");

    // Send data to Firebase
    sendDataToFirebase(totalVolume1, totalVolume2);

    // Print to Serial Monitor
    Serial.print("Sensor 1 Flow Rate: ");
    Serial.print(flowRate1);
    Serial.print(" L/min, ");
    Serial.print("Sensor 2 Flow Rate: ");
    Serial.print(flowRate2);
    Serial.println(" L/min");

    Serial.print("Total Volume 1: ");
    Serial.print(totalVolume1, 2);
    Serial.print(" L, ");
    Serial.print("Total Volume 2: ");
    Serial.print(totalVolume2, 2);
    Serial.println(" L");

    lastMillis = currentMillis;
  }

  // Check button states and control actuators
  if (digitalRead(BUTTON1_PIN) == LOW) {
    digitalWrite(ACTUATOR1_PIN, HIGH);  // Open valve 1
  } else {
    digitalWrite(ACTUATOR1_PIN, LOW);   // Close valve 1
  }

  if (digitalRead(BUTTON2_PIN) == LOW) {
    digitalWrite(ACTUATOR2_PIN, HIGH);  // Open valve 2
  } else {
    digitalWrite(ACTUATOR2_PIN, LOW);   // Close valve 2
  }
}

void sendDataToFirebase(float volume1, float volume2) {
  // Prepare data to send to Firebase
  String postData = "{\"volume1\": " + String(volume1, 2) + ", \"volume2\": " + String(volume2, 2) + "}";

  // Send data to Firebase
  Firebase.setAsync(FIREBASE_PATH, postData);
  if (Firebase.failed()) {
    Serial.print("Firebase failed: ");
    Serial.println(Firebase.error());
  } else {
    Serial.println("Data sent to Firebase!");
  }
}