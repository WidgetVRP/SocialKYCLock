#include <MFRC522.h>  // Eğer bu kütüphane zaten dahil edilmemişse

// ... Diğer kodlar ...


void web_server_setup() {  
  web_server.on("/", handle_root);
  
  web_server.on("/reboot_form", handle_reboot_form);
  web_server.on("/reboot",HTTP_POST, handle_reboot);
  
  web_server.on("/update_form", handle_update_form);
  web_server.on("/update",HTTP_POST, handle_update, handle_update_upload);
  web_server.on("/readRFID", HTTP_GET, handle_readRFID);  // readRFID yolu ekleniyor

  // need a handler for not found
  web_server.begin();
  Serial.println("[Web server] server started");
}

void handle_root() {
    web_server.send(200, "text/html", html_content);  // html_content kullanıyoruz.
}

void handle_reboot_form() {
    String html = String(html_content) + "<h2>Reboot</h2><form method='POST' action='/reboot'><input type='submit' value='Reboot'></form>";  // html_content kullanıyoruz.
    web_server.send(200, "text/html", html);
}

void handle_reboot() {
  // pre_main, post_main tanımlanmadığından, sadece "Rebooting..." mesajını gönderiyoruz.
  String html = "Rebooting...";
  web_server.sendHeader("Connection", "close");
  web_server.sendHeader("Access-Control-Allow-Origin", "*");
  web_server.send(200, "text/html", html);
  ESP.restart();
}

void handle_update_form(){
  // update_form tanımlanmadığından, bu fonksiyonu boş bırakıyoruz.
}

void handle_update(){
  String upload_status;
  if(Update.hasError()){
    upload_status = "Upload failed";
  }
  else {
    upload_status = "Upload success";
  }
  // pre_main, post_main tanımlanmadığından, sadece upload_status mesajını gönderiyoruz.
  String html = upload_status;
  web_server.sendHeader("Connection", "close");
  web_server.sendHeader("Access-Control-Allow-Origin", "*");
  web_server.send(200, "text/html", html);
  ESP.restart();
}

void handle_readRFID() {
  String uid = "";
  // RFID okuyucuyu kontrol et
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    web_server.send(500, "application/json", "{\"error\":\"No card present\"}");
    Serial.println("No card present");  // Hata mesajını seri monitöre yazdır
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    web_server.send(500, "application/json", "{\"error\":\"Failed to read card\"}");
    Serial.println("Failed to read card");  // Hata mesajını seri monitöre yazdır
    return;
  }
  // UID'yi oku ve dizeye dönüştür
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }

  Serial.print("Card UID: ");
  Serial.println(uid); 
  // UID'yi JSON olarak döndür
  String jsonResponse = "{\"uid\":\"" + uid + "\"}";
  web_server.send(200, "application/json", jsonResponse);
}


void handle_update_upload() {
  HTTPUpload& upload = web_server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.setDebugOutput(true);
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin()) { //start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { //true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
    Serial.setDebugOutput(false);
  }
}
