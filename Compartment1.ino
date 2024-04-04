#include <Wire.h>
#include <Servo.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <NewPing.h>
#include <Robojax_L298N_DC_motor.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#define MPU_INTERRUPT 5    // MPU interrupt pin
#define DC_MOTOR_PIN_ENA 9    // Digital pin for controlling DC motor speed
#define DC_MOTOR_PIN_IN1 8    // Digital pin for controlling DC motor direction
#define DC_MOTOR_PIN_IN2 7    // Digital pin for controlling DC motor direction
#define SERVO_PIN_1 10    // Digital pin for controlling servo motor 1
#define SERVO_PIN_2 11    // Digital pin for controlling servo motor 2

#define BIN_CLOSED_ANGLE 0        // Angle indicating bin is closed
#define DISTANCE_THRESHOLD 20     // Threshold distance for ultrasonic sensor to consider bin as filled
#define TRIGGER_PIN 12            // Ultrasonic sensor trigger pin
#define ECHO_PIN 13               // Ultrasonic sensor echo pin
#define SERVO_FINAL_ANGLE 270     // Final angle for servo rotation
#define CHOP_TIME 30000           // Time for chopping waste in milliseconds (30 seconds)

Servo servoMotor1;    // Servo motor 1 object
Servo servoMotor2;    // Servo motor 2 object
Adafruit_MPU6050 mpu; // MPU6050 sensor object
Robojax_L298N_DC_motor motor(DC_MOTOR_PIN_IN1, DC_MOTOR_PIN_IN2, DC_MOTOR_PIN_ENA, DC_MOTOR_PIN_IN2, DC_MOTOR_PIN_IN3, DC_MOTOR_PIN_ENB, true); // Motor object
NewPing sonar(TRIGGER_PIN, ECHO_PIN);

bool binClosed = true;

const char *ssid = "*******";
const char *password = "*******";
const char *server = "api.thingspeak.com";
const String apiKey = "*******";

WiFiClient client;

void setup() {
  Serial.begin(9600);

  pinMode(DC_MOTOR_PIN_ENA, OUTPUT);
  pinMode(DC_MOTOR_PIN_IN1, OUTPUT);
  pinMode(DC_MOTOR_PIN_IN2, OUTPUT);
  servoMotor1.attach(SERVO_PIN_1);
  servoMotor2.attach(SERVO_PIN_2);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  pinMode(MPU_INTERRUPT, INPUT);
  digitalWrite(MPU_INTERRUPT, LOW);

  attachInterrupt(digitalPinToInterrupt(MPU_INTERRUPT), mpuInterrupt, RISING);

  connectWiFi();
}

void loop() {
  if (binClosed) {
    int distance = sonar.ping_cm();
    if (distance > 0 && distance <= DISTANCE_THRESHOLD) {
      startDCMotor();
      delay(CHOP_TIME);
      stopDCMotor();
      rotateServos(SERVO_FINAL_ANGLE);

      // Send data to ThingSpeak
      sendToThingSpeak(distance);
    }
  }
}

void mpuInterrupt() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if (abs(g.gyro.z) < 50) {
    binClosed = true;
  } else {
    binClosed = false;
  }
}

void startDCMotor() {
  digitalWrite(DC_MOTOR_PIN_IN1, HIGH);
  digitalWrite(DC_MOTOR_PIN_IN2, LOW);
  analogWrite(DC_MOTOR_PIN_ENA, 150); // Adjust speed as needed (0-255)
}

void stopDCMotor() {
  digitalWrite(DC_MOTOR_PIN_IN1, LOW);
  digitalWrite(DC_MOTOR_PIN_IN2, LOW);
}

void rotateServos(int angle) {
  servoMotor1.write(angle);
  servoMotor2.write(angle);
  delay(1000);
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void sendToThingSpeak(int distance) {
  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(distance);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
  client.stop();
}
