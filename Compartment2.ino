
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <WiFiClient.h>


#define DHT_PIN D2
#define MOISTURE_PIN A0
#define MQ135_PIN D3
#define WATER_PUMP_PIN D4

#define DHTTYPE DHT22

const char* ssid = "FABLAB";
const char* password = "********";


const char* apiKey = "**************";
const char* channelId = "*******";

DHT dht(DHT_PIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(MQ135_PIN, INPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi...");
    delay(500);
  }
  Serial.println("Connected to the WiFi network");

  // Initialize DHT sensor
  dht.begin();
}

void loop() {
  delay(2000);  

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  int moistureValue = analogRead(MOISTURE_PIN);
  // Assuming moisture sensor output is inversely proportional to soil moisture
  float soilMoisture = map(moistureValue, 0, 1023, 100, 0);

  int gasValue = digitalRead(MQ135_PIN);
  // Assuming gas sensor output HIGH indicates presence of CO2
  bool co2Present = (gasValue == HIGH);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoisture);
  Serial.println(" %");
  Serial.print("CO2 Presence: ");
  Serial.println(co2Present ? "Detected" : "Not Detected");

  if (temperature >= 50 && humidity >= 50 && !co2Present && soilMoisture < 50) {
    Serial.println("Composting process is going properly.");
    // Turn on water pump
    digitalWrite(WATER_PUMP_PIN, HIGH);
    Serial.println("Watering the compost.");
  } else {
    Serial.println("Composting process may not be going properly. Check conditions.");
    // Turn off water pump
    digitalWrite(WATER_PUMP_PIN, LOW);
    Serial.println("Stopping watering.");
  }


String url = "http://api.thingspeak.com/update?api_key=";
  url += apiKey;
  url += "&field1=";
  url += String(temperature);
  url += "&field2=";
  url += String(humidity);
  url +="&field3=";
  url +=String(soilMoisture);
  url +="&field4=";
  url += String(co2Present);

// Send the HTTP GET request to ThingSpeak
  if (client.connect("api.thingspeak.com", 80)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: api.thingspeak.com\r\n" +
                 "Connection: close\r\n\r\n");
    delay(500); 
    client.stop(); 
    Serial.println("Data sent to ThingSpeak successfully");
  } else {
    Serial.println("Failed to connect to ThingSpeak");
  }


  delay(15000); 

}