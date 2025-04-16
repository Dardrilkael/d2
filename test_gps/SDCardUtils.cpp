#include "SDCardUtils.h"
#include <SD.h>
#include <sd_defines.h>
#include <sd_diskio.h>

#include "SD.h"
#include "SPI.h"
#include <ArduinoJson.h>



char buffer[BUFFER_SIZE];

void SD_BLINK(int interval) {
    for (int i = 0; i < interval / 500; i++) {
        // digitalWrite(LED2, i % 2);
        delay(500 + ((i % 2 == 0) && (i % 3 > 0)) * 1000);
    }
}

void initSdCard() {
    SPI.begin(clockPin, misoPin, mosiPin);
    while (!SD.begin(chipSelectPin, SPI)) {
        OnDebug(Serial.printf("\n  - Cartão não encontrado. tentando novamente em %d segundos ...", 2);)
        SD_BLINK(2000);
        break;
    }
    OnDebug(Serial.printf("\n  - Leitor de Cartão iniciado com sucesso!.\n");)
}

void createDirectory(const char *directory) {
    OnDebug(Serial.printf("\n  - Tentando Criar novo diretorio: %s.", directory);)
    if (!SD.exists(directory)) {
        if (SD.mkdir(directory)) {
            OnDebug(Serial.printf("\n     - Diretorio criado com sucesso!");)
        } else {
            OnDebug(Serial.printf("\n     - Falha ao criar diretorio.");)
        }
        return;
    }
    OnDebug(Serial.printf("\n     - Diretorio já existe.");)
}

void parseMQTTString(const char *mqttString, char *username, char *password, char *broker, int &port) {
    if (memcmp(mqttString, "mqtt://", 7) != 0) {
        printf("Invalid MQTT string format!\n");
        return;
    }
    int size = strlen(mqttString) + 1;
    char *ptr = new char[size - 7];
    strlcpy(ptr, mqttString + 7, size - 7);
    strlcpy(username, strtok(ptr, ":"), 64);
    strlcpy(password, strtok(NULL, "@"), 64);
    strlcpy(broker, strtok(NULL, ":"), 64);
    port = atoi(strtok(NULL, ""));
    delete[] ptr;
}

void parseWIFIString(const char *wifiString, char *ssid, char *password) {
    int size = strlen(wifiString) + 1;
    char *ptr = new char[size];
    strlcpy(ptr, wifiString, size);
    strlcpy(ssid, strtok(ptr, ":"), 64);
    strlcpy(password, strtok(NULL, ""), 64);
    delete[] ptr;
}

bool loadConfiguration(fs::FS &fs, const char *filename, Config &config, std::string &configJson) {
    SPI.begin(clockPin, misoPin, mosiPin);
    static int attemptCount = 0;
    bool success = false;
    Serial.printf("\n - Iniciando leitura do arquivo de configuração %s (tentativa: %d)", filename, attemptCount + 1);
    if (SD.begin(chipSelectPin, SPI)) {
        File file = fs.open(filename);
        StaticJsonDocument<512> doc;
        if (file) {
            DeserializationError error = deserializeJson(doc, file);
            if (!error) {
                strlcpy(config.station_name, doc["SLUG"] | "est000", sizeof(config.station_name));
                parseWIFIString(doc["WIFI"], config.wifi_ssid, config.wifi_password);
                parseMQTTString(doc["MQTT_HOST"], config.mqtt_username, config.mqtt_password, config.mqtt_server, config.mqtt_port);
                strlcpy(config.topic, doc["MQTT_TOPIC"] | "topic/drone00", sizeof(config.topic));
                config.interval = doc["INTERVAL"] | 60000;
                file.close();
                success = true;
                serializeJson(doc, configJson);
                Serial.printf("\n - Variáveis de ambiente carregadas com sucesso!");
                Serial.printf("\n - %s\n", configJson.c_str());
                return true;
            }
            Serial.printf("\n - [ ERROR ] Formato inválido (JSON)\n");
            Serial.println(error.c_str());
        }
        Serial.printf("\n - [ ERROR ] Arquivo de configuração não encontrado\n");
    } else {
        Serial.printf("\n - [ ERROR ] Cartão SD não encontrado.\n");
    }
    Serial.printf("\n - Proxima tentativa de re-leitura em %d segundos ... \n\n\n", (RETRY_INTERVAL / 1000));
    attemptCount++;
    SD_BLINK(RETRY_INTERVAL);
    return false;
}

void createFile(fs::FS &fs, const char *path, const char *message) {
    OnDebug(Serial.printf("Salvando json no cartao SD: %s\n.", path);)
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        OnDebug(Serial.println(" - Falha ao encontrar cartão SD.");)
        return;
    }
    if (file.print(message)) {
        OnDebug(Serial.println(" - sucesso.");)
    } else {
        OnDebug(Serial.println("- Falha ao salvar.");)
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    OnDebug(Serial.printf(" - Salvando dados no cartao SD: %s\n", path);)
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        OnDebug(Serial.println(" - Falha ao encontrar cartão SD");)
        return;
    }
    if (file.print(message)) {
        OnDebug(Serial.println(" - Nova linha salva com sucesso.");)
    } else {
        OnDebug(Serial.println(" - Falha ao salvar nova linha");)
    }
    file.close();
}

void storeMeasurement(String directory, String fileName, const char *payload) {
    String path = directory + "/" + fileName + ".txt";
    if (!SD.exists(directory)) {
        if (SD.mkdir(directory)) {
            OnDebug(Serial.println(" - Diretorio criado com sucesso!");)
        } else {
            OnDebug(Serial.println(" - Falha ao criar diretorio de metricas.");)
        }
    }
    appendFile(SD, path.c_str(), payload);
}

const char* listDirectory(File& dir, size_t limit) {
    buffer[0] = '\0'; // Clear buffer at the beginning of each function call
    size_t writtenLength = 0; // Initialize writtenLength to 0

    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        size_t entryNameLength = strlen(entry.name());
        if (writtenLength + entryNameLength + 2 > limit) {
            dir.seek(dir.position() - entryNameLength); // Move back the file pointer
            return buffer; // Return the directory list buffer
        }

        strcat(buffer, entry.name()); // Append entry name to buffer
        strcat(buffer, "\n"); // Append newline character
        entry.close();

        writtenLength += entryNameLength + 1; // Update writtenLength to include the entry name and newline character
    }
    return buffer; // Return the directory list buffer
}

void storeLog(const char *payload) {
    String path = "/logs/boot.txt";
    File file = SD.open(path, FILE_APPEND);
    if (file) {
        file.print(payload);
    }
    file.close();
}
