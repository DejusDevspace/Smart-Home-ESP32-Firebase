#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <ESP32Servo.h>

// Insert your network credentials
#define WIFI_SSID "removed"
#define WIFI_PASSWORD "removed"

// Insert Firebase project API Key
#define API_KEY "removed"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "removed" 

// ----------------  Pins --------------------- //
#define led_pin 5
#define pir_pin 35

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
// int count = 0;
bool signupOK = false;

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows
int lcdPower = 7;

Servo myServo;
int servoPin = 13;

String tvText;

void lcdDisplayText() {
  lcd.clear();              // clear display
  lcd.setCursor(0, 0);      // move cursor to   (0, 0)
  lcd.print("Welcome Home");       // print message at (0, 0)
  lcd.setCursor(6, 1);      // move cursor to   (6, 1)
  lcd.print("TV ON");
}


void setup(){
  Serial.begin(115200);

  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(led_pin, OUTPUT);
  pinMode(pir_pin, INPUT);

  myServo.attach(servoPin);
}

void loop() {

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1500 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    // Read the pir value from the pir pin
    int pir_value = digitalRead(pir_pin);

    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setInt(&fbdo, "home/PIR", pir_value)){
      int pirData = fbdo.intData();
      Serial.print("PIR sensor value: ");
      Serial.print(pirData);
      Serial.println(" ");
    } else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
    }
    // count++;
    
    // Turn the LED on or off from the app
    if (Firebase.RTDB.getBool(&fbdo, "home/LED")) {
      bool ledState = fbdo.boolData();

      Serial.print("LED state: ");
      Serial.print(ledState);
      Serial.println(" ");
      digitalWrite(led_pin, ledState);
    } 
    else {
      digitalWrite(led_pin, LOW);
    }

    // Turn the LCD on or off from the app
    if (Firebase.RTDB.getBool(&fbdo, "home/TV")) {
      bool tvState = fbdo.boolData();
      Serial.print("TV state: ");
      Serial.print(tvState);
      Serial.println(" ");
      lcd.clear();
      if (tvState) {
        // lcdDisplayText();
        if (Firebase.RTDB.getString(&fbdo, "home/LCD")) {
          tvText = fbdo.stringData();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(tvText);
        }
      } else {
      lcd.clear();
      lcd.print("");
      }
    }

    // Open or close the door from the app
    if (Firebase.RTDB.getBool(&fbdo, "home/Door")) {
      bool doorState = fbdo.boolData();
      Serial.print("Door state: ");
      Serial.print(doorState);
      Serial.println(" ");
      if (doorState) {
        myServo.write(0);
      } else {
        myServo.write(90);
    }
  }
}
}
