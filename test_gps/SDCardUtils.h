#pragma once
#include "FS.h"


// Pin Definitions
const int chipSelectPin = 32;
const int mosiPin = 23;
const int misoPin = 27;
const int clockPin = 25;    
const int RETRY_INTERVAL = 5000;

#define OnDebug(x) x

// Struct to hold configuration data
struct Config {
    char station_name[64];
    char wifi_ssid[64];
    char wifi_password[64];
    char mqtt_username[64];
    char mqtt_password[64];
    char mqtt_server[64];
    char topic[128];
    int mqtt_port;
    unsigned long interval;
};

// Function declarations
void SD_BLINK(int interval);
void initSdCard();
void createDirectory(const char * directory);
void parseMQTTString(const char *mqttString, char *username, char *password, char *broker, int &port);
void parseWIFIString(const char *wifiString, char *ssid, char *password);
bool loadConfiguration(fs::FS &fs, const char *filename, Config &config, std::string& configJson);
void createFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void storeMeasurement(String directory, String fileName, const char *payload);
const char* listDirectory(File& dir, size_t limit);
void storeLog(const char *payload);

#define BUFFER_SIZE 512
extern char buffer[BUFFER_SIZE];
