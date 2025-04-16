#include <TinyGPS++.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "TimeManager.h"
#include "SDCardUtils.h"
#include "GasSensor.h"
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_AHTX0.h>
#include "SD.h"
#include <string>
#define GPS_BAUDRATE 9600  // The default baudrate of NEO-6M is 9600
#define RX_PIN 16
#define TX_PIN 17

/*
const char* ssid = "Gabriel";
const char* password = "2014072276";
const char* mqtt_server = "192.168.0.223";
const int mqtt_port = 1883;
*/
Config configuration{"drone","Gabriel","2014072276","telemetria","kancvx8thz9FCN5jyq","broker.gpicm-ufrj.tec.br","/prefeituras/macae/estacoes/estdrone",1883,10000};
// Struct to hold configuration data

const int MG811_PIN = 34;   // Analog input pin for MG811 sensor
const int MQ4_PIN = 35;     // Analog input pin for MQ-4 sensor
const int MQ7_PIN = 26;     // Analog input pin for MQ-7 sensor
const int I2C_SDA = 21;     // Common SDA pin for BMP180 and AHT10
const int I2C_SCL = 22;     // Common SCL pin for BMP180 and AHT10
int before =0;
WiFiClient espClient;
PubSubClient client(espClient);
TinyGPSPlus gps;
TimeManager timeManager;

Adafruit_BMP085 bmp;    
Adafruit_AHTX0 aht;    

GasSensor mq4Sensor(MQ4_PIN, 997.361, -2.81457, 20000.0, 17940.3421723, 3.3); // Example parameters for MQ-4 sensor
GasSensor mq7Sensor(MQ7_PIN, 89.295, -1.58996, 10000.0, 10000, 3.3); // Example parameters for MQ-7 sensor
CO2Sensor mg811Sensor(MG811_PIN);
std::string jsonConfig = "";
bool began = false;
void setup() {
  delay(250);
  Serial.begin(115200);
  Serial2.begin(GPS_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  Wire.begin(I2C_SDA, I2C_SCL);

  initSdCard();
  bool loadedSD = false;
  //while(!loadedSD) loadedSD = loadConfiguration(SD, "/config.txt", configuration, jsonConfig);
  began=bmp.begin();
  while(!began && 0) {
    began=bmp.begin();
    Serial.println("BMP180 not found. Check wiring or I2C address.");
  }


  if (!aht.begin()) {
    Serial.println("AHT10 not found. Check wiring or I2C address.");
  }
  // Connect to Wi-Fi
  WiFi.begin(configuration.wifi_ssid, configuration.wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
 
  // Initialize MQTT
  Serial.println(configuration.mqtt_server);
  client.setServer(configuration.mqtt_server, configuration.mqtt_port);
  reconnectMQTT();
  client.publish(configuration.topic, "Inicando O Esp32");
  //timeManager.updateEpoch(1000);
}
sensors_event_t humidity, temp;
void loop() {
  int now = millis();
  if (now - before > configuration.interval){
    before =now;
  
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  Serial.println("\nlooping");
  // Read GPS data
  double latitude  = 0.0/0.0;
  double longitude = 0.0/0.0;
  double altitude  = 0.0/0.0;
  double speed     = 0.0/0.0;
  double date      = 0.0/0.0;
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    gps.encode(c);
  }

    
    if (gps.location.isValid()) {
       latitude = gps.location.lat();
       longitude = gps.location.lng();
    }
       Serial.println(longitude);
    if(gps.altitude.isValid()){
       altitude =gps.altitude.meters();
    }
    
    if(gps.date.isValid() && gps.time.isValid()){
      timeManager.updateEpoch(dateTimeToTimestamp(gps.date.year(), gps.date.month(), gps.date.day(),
              gps.time.hour(), gps.time.minute(), gps.time.second()-10));
     
      
            Serial.printf("Original: %04d-%02d-%02d %02d:%02d:%02d\n",gps.date.year(), gps.date.month(), gps.date.day(),
              gps.time.hour(), gps.time.minute(), gps.time.second());
    }
  

    char unixDateTime[20]{0};
    long timeStamp = timeManager.getCurrentTime();
    timestampToDateTime(timeStamp,unixDateTime,20);
    Serial.printf("Local timeStamp: %ld\n",timeStamp);



    float c0 = 0.0f;//mq7Sensor.readConcentration();
    float ch4 = mq4Sensor.readConcentration();
    float co2 = 0.0f;//mg811Sensor.readConcentration();


    float pressure = 0.0/0.0;


   if (!aht.getEvent(&humidity, &temp)) {
      Serial.println("Error reading AHT10 data.");
    }
    if(began)
    pressure =  bmp.readPressure() / 100.0F;
    

    float v_humidity = humidity.relative_humidity;
    float v_temperature = temp.temperature;//isnan(pressure)

    char buffer[256]{0};
    sprintf(buffer, "%.7f,%.7f, %.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f \n", latitude, longitude, altitude, unixDateTime, c0, ch4,co2,v_humidity,pressure,v_temperature);
    Serial.printf("Mqtt data: %s\n",buffer);
    client.publish(configuration.topic, buffer);
    storeMeasurement("/senha","name",buffer);


  // Check if GPS data is being received
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS data received: check wiring"));
  }
  }
 
}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    printf("%s - %s",configuration.mqtt_username,configuration.mqtt_password);
    if (client.connect("ESP32Client",configuration.mqtt_username,configuration.mqtt_password)) {
      Serial.println("connected");
      break;
      // Subscribe to a topic if needed
      // client.subscribe("your/topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

unsigned long gpsToUnixTimestamp(int year, int month, int day, int hour, int minute, int second) {
  // Define the GPS epoch offset (seconds from Unix epoch to GPS epoch)
  const unsigned long GPS_EPOCH_OFFSET = 315964800;

  // Create a tm structure for the GPS time
  struct tm t;
  t.tm_year = year - 1900; // tm_year is year since 1900
  t.tm_mon = month - 1;    // tm_mon is month (0-11)
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  t.tm_isdst = -1;        // Not using Daylight Saving Time

  // Convert tm to time_t (Unix timestamp)
  time_t epoch = mktime(&t);

  // Add the GPS epoch offset to convert to Unix timestamp
  return epoch + GPS_EPOCH_OFFSET;
}


