#include <SimpleDHT.h>

const int photoresistorPin = A0;
const int soilMoistureSensorPin = A1;
const int pumpPin = 7; // (change if necessary)

int pinDHT22 = 2;
SimpleDHT22 dht22;

// Constants for the algorithm
const int MaxDelay = 172800000; // Max of 48 hours between watering
const int MinDelay = 7200000; // Min of 2 hours between watering
const int CriticalValue = 200; // Critical moisture level (change if necessary)

// Weights for each sensor (change if necessary)
const float DirtWeight = 0.4;
const float TempWeight = 0.2;
const float HumWeight = 0.2;
const float LightWeight = 0.2;

// Maximum values for each sensor
const int DirtMaxValue = 1023;
const int TempMaxValue = 100; // Assuming 100F = max temperature
const int HumMaxValue = 100;  // Assuming 100% = max humidity
const int LightMaxValue = 1023;

// Variables to store the last watering time
unsigned long LastWaterTime = 0;

unsigned long LastTestTime = 0; // For testing the pump

extern float TotalPercent;

// Function to map a float value
long mapfloat(float x, float in_min, float in_max, long out_min, long out_max) {
  return (long)((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

// Function to control watering
void water(float TotalPercent) {
  int wateringDurationMs = mapfloat(TotalPercent, 0, 1, 1000, 4000);

  Serial.print("Watering the plant for ");
  Serial.print(wateringDurationMs);
  Serial.println(" milliseconds.");

  digitalWrite(pumpPin, HIGH);
  delay(wateringDurationMs); // Keep the pump on for the calculated duration
  digitalWrite(pumpPin, LOW);
}

void setup() {
  pinMode(pumpPin, OUTPUT); // Set the pump pin as an output
  Serial.begin(9600);
}

void loop() {
    // Check if data is available to read from the serial port
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n'); // Read the data from serial port
        input.trim(); // Remove any whitespace

        if (input == "WATER_ON" || input == "FORCE_WATER") {
            // If the input is "WATER_ON" or "FORCE_WATER", activate the pump for 5 seconds
            digitalWrite(pumpPin, HIGH);
            delay(5000); // Pump runs for 5000 milliseconds (5 seconds)
            digitalWrite(pumpPin, LOW);
            Serial.println("Pump activated for 5 seconds.");
        }
    }
  
  int lightLevel = analogRead(photoresistorPin);
  int soilMoistureLevel = 1023 - analogRead(soilMoistureSensorPin);

  float temperatureC = 0;
  float humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(pinDHT22, &temperatureC, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT22 failed, err="); Serial.println(err);delay(1000);
    return;
  }
  float temperatureF = temperatureC * 9.0 / 5.0 + 32;

  // Send sensor data over Serial
  Serial.print(lightLevel);
  Serial.print(",");
  Serial.print(soilMoistureLevel);
  Serial.print(",");
  Serial.print(temperatureF);
  Serial.print(",");
  Serial.println(humidity);

  checkAndWater(soilMoistureLevel, temperatureF, humidity, lightLevel);

  delay(2000);
}

void checkAndWater(int soilMoisture, float temperature, float humidity, int light) {
  float DirtPercent = DirtWeight * (1 - (float)(DirtMaxValue - soilMoisture) / DirtMaxValue);
  float TempPercent = TempWeight * ((TempMaxValue - temperature) / TempMaxValue);
  float HumPercent = HumWeight * (1 - (float)(HumMaxValue - humidity) / HumMaxValue);
  float LightPercent = LightWeight * (1 - (float)(LightMaxValue - light) / LightMaxValue);

  float TotalPercent = DirtPercent + TempPercent + HumPercent + LightPercent; // Use float instead of int
  long Delay = mapfloat(TotalPercent, 0, 1, MinDelay, MaxDelay); // Use mapfloat to calculate delay
  bool DoStandardWater = (millis() - LastWaterTime > Delay);
  bool DoEmergencyWater = (soilMoisture <= CriticalValue && millis() - LastWaterTime > MinDelay);

  if (DoStandardWater || DoEmergencyWater) {
    water(TotalPercent); // Pass TotalPercent to water function
    LastWaterTime = millis();
  }
}
