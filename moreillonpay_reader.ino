#include <WebSocketsServer.h> 

/*
 * Board type: ESP32 Dev module
 */

// Libraries
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include "SSD1306.h"

#include "credentials.h";
#include "moreillonpay_credentials.h";

// Import fonts and images for the display
#include "font.h"
#include "images.h"

// API parameters
#define API_URL "http://192.168.1.2:7342/transaction"
#define TRANSACTION_AMOUNT "-60"
#define TRANSACTION_DESCRIPTION "JPY 60 Coffee"

// Wifi settings
#define WIFI_CONNECTION_TIMEOUT 5000

// IO
#define SDA_PIN 5
#define SCL_PIN 4

#define SPI_SCK_PIN 14
#define SPI_MISO_PIN 12
#define SPI_MOSI_PIN 13

// Double check if working with RST on DPIO2
#define RFID_RST_PIN 16
#define RFID_SS_PIN 15
#define BUZZER_PIN 25

// RFID parameters
#define UID_SIZE 4

// Display parameters
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_MESSAGE_LENGTH 3000
#define DISPLAY_INVERSION_PERIOD 60000

// Buzzer parameters
#define BUZZER_CHANNEL 0
#define BUZZER_RESOLUTION 8

// RFID parameters
#define RFID_SELF_TEST_PERIOD 5000

// Create MFRC522 instance
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN); 

// Create display instance
SSD1306 display(0x3c, SDA_PIN, SCL_PIN);

// Web server
WebServer web_server(80);
WebSocketsServer webSocket = WebSocketsServer(81);  // Port 81'de bir WebSocket sunucusu başlatın


// Global variables for wifi management
boolean wifi_connected = false;
long wifi_disconnected_time = 0;

void readAndSendRFIDData() {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        mfrc522.PICC_DumpToSerial(&(mfrc522.uid));  // Kartın içeriğini seri port üzerinden yazdır

        // UID'yi oku ve dizeye dönüştür
        String uid = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            uid += String(mfrc522.uid.uidByte[i], HEX);
        }

        // Kartın içindeki diğer verileri okuyun (örnek olarak 1. sektörün 0. bloğunu okuyun)
        byte block;
        byte len;
        MFRC522::MIFARE_Key key;
        for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;  // varsayılan anahtarı kullan

        block = 4;  // 1. sektörün 0. bloğu
        len = 16;
        byte buffer1[len];
        MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); //blok erişim yetkisi
        if (status != MFRC522::STATUS_OK) {
            Serial.print(F("Authentication failed: "));
            Serial.println(mfrc522.GetStatusCodeName(status));
            return;
        }
        status = mfrc522.MIFARE_Read(block, buffer1, &len);
        if (status != MFRC522::STATUS_OK) {
            Serial.print(F("Reading failed: "));
            Serial.println(mfrc522.GetStatusCodeName(status));
            return;
        }

        // Verileri bir dizeye dönüştürün
        String blockData = "";
        for (byte i = 0; i < 16; i++) {
            blockData += String(buffer1[i], HEX);
        }

        // Verileri JSON olarak hazırlayın ve web soketi üzerinden gönderin
        String message = "{\"uid\":\"" + uid + "\", \"blockData\":\"" + blockData + "\"}";
        webSocket.broadcastTXT(message);  // Tüm istemcilere verileri gönderin
    }
}



void setup() {
  delay(100);
	Serial.begin(115200);

  wifi_setup();
  display_setup();
  buzzer_setup();
  rfid_setup();
  web_server_setup();
  webSocket.begin();
}

void loop() {
    webSocket.loop();
    // Check for WiFi
    if(WiFi.status() != WL_CONNECTED){
        // Wifi disconnected
        if(wifi_connected){
            // Acknowledge disconnection
            wifi_connected = false;
            wifi_disconnected_time = millis();
            Serial.println(F("[WIFI] disconnected"));
        }
        wifi_reset_if_timout();
        display_connecting();
    }
    else {
        // Wifi connected
        if(!wifi_connected){
            // Acknowledge connection
            wifi_connected = true;
            Serial.print(F("[WIFI] connected, IP: "));
            Serial.println(WiFi.localIP());
            display_connected();
            delay(2000);
            display_price();
        }
        readAndSendRFIDData();  // RFID kartını oku ve verileri gönder
    }
    invert_display_periodically();
    rfid_periodic_self_test();
    web_server.handleClient();
}
