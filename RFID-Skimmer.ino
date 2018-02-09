#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

ADC_MODE(ADC_VCC);

static char DEBUGMODE=0;

//helpers for debugging timings
static inline int32_t asm_ccount(void) {
    int32_t r; asm volatile ("rsr %0, ccount" : "=r"(r)); return r; }

static inline void asm_nop() {
    asm volatile ("nop"); }

// Variables used for temporary storage
static uint8_t data[64];
static uint8_t value[10];
static String webResponse;

// Variables used for state keeping
static uint8_t delayed_setup = 1;
static uint8_t state=1;
static uint8_t active_id=1;

uint8_t  vendor_active=0;
uint32_t ID_active=0;


void prepare_data(uint32_t ID, uint32_t VENDOR){
  value[0] = (VENDOR>>4) & 0XF;
  value[1] = VENDOR & 0XF;
  for (int i=1; i<8; i++){
    value[i+2] = (ID>>(28-i*4)) &0xF;
  }
  
  for (int i=0; i<9; i++) data[i]=1; //header
  for (int i=0; i<10; i++) {         //data
    for (int j=0; j<4; j++) {
      data[9 + i*5 +j] = value[i] >> (3-j) & 1;
    }
    data[9 + i*5 + 4] = ( data[9 + i*5 + 0]
                        + data[9 + i*5 + 1]
                        + data[9 + i*5 + 2]
                        + data[9 + i*5 + 3]) % 2;
  }
  for (int i=0; i<4; i++) {          //checksum
    int checksum=0;
    for (int j=0; j<10; j++) {
      checksum += data[9 + i + j*5];
    }
    data[i+59] = checksum%2;
  }
  data[63] = 0;                      //footer
  
  /*
  delay(10);
  Serial.println();
  for (int i=0; i<64; i++) {
    Serial.printf("%d", data[i]);
    if (i>=8 && (i+2)%5==0) Serial.printf("\r\n    ");
  }
  Serial.println();
  delay(10);
  */
}

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);

void handleRoot() {
  webResponse = "<!DOCTYPE html><meta charset=UTF-8><title>RFID Ugly panel</title><script>function httpGetAsync(e,t){var n=new XMLHttpRequest;n.onreadystatechange=function(){4==n.readyState&&200==n.status&&t&&t(n.responseText)},n.open(\"GET\",e,!0),n.send(null)}window.onload=function(){httpGetAsync(\"/api\",setup)};function setup(e){e=JSON.parse(e);for(var t=document.getElementById(\"available\"),n=0;n<e.rfid.length;n++){var o=document.createElement(\"option\");o.text=e.rfid[n].name+\" (\"+e.rfid[n].vendor+\"|\"+e.rfid[n].uid+\")\",o.value=e.rfid[n].id,t.options.add(o,o.value)}for(n=0;n<t.length;n++)if(t.options[n].value==e.active_id){t.options.selectedIndex=n;break}}function update(){var e=document.getElementById(\"available\");return console.log(e.options[e.options.selectedIndex].value),httpGetAsync(\"/api?rfid=\"+e.options[e.options.selectedIndex].value),!1}</script><style>button,input,select{display:block;margin:1px}button,input{min-width:200px}body{background-color:#eee}</style><h1>Welcome to<br><strike>Admin</strike>Ugly panel</h1><form action=/api onsubmit=\"return update()\"><select id=available><option value=0>Disabled</select><input type=submit value=Update></form><button id=wifi onclick='location.href=\"/wifi\"'>WiFi Setup</button> <button id=rfid onclick='location.href=\"/rfid\"'>RFID Setup</button>";
  server.send(200, "text/html", webResponse.c_str());
}

void handleWifi() {
  webResponse = "<!DOCTYPE html><meta charset=UTF-8><title>RFID Ugly panel</title><script>function httpGetAsync(e,t){var n=new XMLHttpRequest;n.onreadystatechange=function(){4==n.readyState&&200==n.status&&t&&t(n.responseText)},n.open(\"GET\",e,!0),n.send(null)}window.onload=function(){httpGetAsync(\"/api\",setup)};function reload(){location.reload()}function setup(e){e=JSON.parse(e);for(var t=document.getElementById(\"wifis\"),n=0;n<e.wifi.length;n++){var o=document.createElement(\"option\");o.text=e.wifi[n].ssid+\" (\"+e.wifi[n].pass.length+\")\",o.value=e.wifi[n].id,t.options.add(o,o.value)}}function remove(){var e=document.getElementById(\"wifis\");return console.log(e.options[e.options.selectedIndex].value),httpGetAsync(\"/api?remove=wifi&id=\"+e.options[e.options.selectedIndex].value),e.options.remove(e.options.selectedIndex),!1}function add(){return console.log(\"/api?add=wifi&ssid=\"+document.getElementById(\"ssid\").value+\"&pass=\"+document.getElementById(\"pass\").value),httpGetAsync(\"/api?add=wifi&ssid=\"+document.getElementById(\"ssid\").value+\"&pass=\"+document.getElementById(\"pass\").value,reload),!1}</script><style>button,input,select{display:block;margin:1px}button,input{min-width:200px}body{background-color:#eee}</style><form action=/api onsubmit=\"return remove()\"><select id=wifis></select><input type=submit value=Delete></form><form action=/api onsubmit=\"return add()\"><span>SSID: </span><input id=ssid> <span>PASS: </span><input type=password id=pass> <input type=submit value=Add></form><button id=wifi onclick='location.href=\"/\"'>Home</button> <button id=rfid onclick='location.href=\"/rfid\"'>RFID Setup</button>";
  server.send(200, "text/html", webResponse.c_str());
}

void handleRfid() {
  webResponse = "<!DOCTYPE html><meta charset=UTF-8><title>Control Room</title><script>function httpGetAsync(e,t){var n=new XMLHttpRequest;n.onreadystatechange=function(){4==n.readyState&&200==n.status&&t&&t(n.responseText)},n.open(\"GET\",e,!0),n.send(null)}window.onload=function(){httpGetAsync(\"/api\",setup)};function reload(){location.reload()}function setup(e){e=JSON.parse(e);for(var t=document.getElementById(\"rfids\"),n=0;n<e.rfid.length;n++){var d=document.createElement(\"option\");d.text=e.rfid[n].name+\"(\"+e.rfid[n].vendor+\"|\"+e.rfid[n].uid+\")\",d.value=e.rfid[n].id,t.options.add(d,d.value)}}function remove(){var e=document.getElementById(\"rfids\");return console.log(e.options[e.options.selectedIndex].value),httpGetAsync(\"/api?remove=rfid&id=\"+e.options[e.options.selectedIndex].value),e.options.remove(e.options.selectedIndex),!1}function add(){return console.log(\"/api?add=rfid&uid=\"+document.getElementById(\"uid\").value+\"&vendor=\"+document.getElementById(\"vendor\").value+\"&name=\"+document.getElementById(\"name\").value),httpGetAsync(\"/api?add=rfid&uid=\"+document.getElementById(\"uid\").value+\"&vendor=\"+document.getElementById(\"vendor\").value+\"&name=\"+document.getElementById(\"name\").value,reload),!1}</script><style>button,input,select{display:block;margin:1px}button,input{min-width:200px}body{background-color:#eee}</style><form action=/api onsubmit=\"return remove()\"><select id=rfids></select><input type=submit value=Delete></form><form action=/api onsubmit=\"return add()\"><span>Vendor (can be 0): </span><input id=vendor> <span>ID: </span><input id=uid> <span>Name: </span><input id=name> <input type=submit value=Add></form><button id=home onclick='location.href=\"/\"'>Home</button> <button id=wifi onclick='location.href=\"/wifi\"'>WiFi Setup</button>";
  server.send(200, "text/html", webResponse.c_str());
}

void handleApi() {
  File configFile = SPIFFS.open("/rfid.conf", "r");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &config = jsonBuffer.parseObject(configFile);
//  config.printTo(Serial);
  if (server.hasArg("debug")){
    DEBUGMODE = (server.arg("debug")=="true" || server.arg("debug")=="1") ? 1:0;
    config["debug"]=DEBUGMODE;
    configFile.close();
    configFile = SPIFFS.open("/rfid.conf", "w");
    config.printTo(configFile);
  }


  if (server.hasArg("rfid")){
    if (server.arg("rfid")=="temp"){
      ID_active=server.arg("uid").toInt();
      vendor_active=server.arg("vendor").toInt();
      prepare_data(ID_active, vendor_active);
    }
    else {
      active_id = server.arg("rfid").toInt();
      if(active_id){
        int i=0;
        for (i=0; i<config["rfid"].size(); i++){
          if (config["rfid"][i]["id"]==active_id){
            ID_active=config["rfid"][i]["uid"];
            vendor_active=config["rfid"][i]["vendor"];
            prepare_data(ID_active, vendor_active);
            break;
          }
        }
        if (i==config["rfid"].size()){
          active_id=0;
        }
      }
      config["active_id"]=active_id;
      configFile.close();
      configFile = SPIFFS.open("/rfid.conf", "w");
      config.printTo(configFile);
    }
    
    
  }
  
  if (server.hasArg("add"))
  {
    if (server.arg("add")=="wifi"){
      if (server.hasArg("ssid") && server.hasArg("pass") && server.arg("pass").length()>7 ){
        JsonArray &wifis = config["wifi"];
        int i=0, idmax=0;
        for (i=0; i< wifis.size(); i++){
          if (wifis[i]["ssid"] == server.arg("ssid")) break;
          idmax = (wifis[i]["id"] < idmax)? idmax:wifis[i]["id"];

        }
        if (i==wifis.size()){
          JsonObject &newWiFi = jsonBuffer.createObject();
          newWiFi["id"]=idmax+1;
          newWiFi["ssid"]=server.arg("ssid");
          newWiFi["pass"]=server.arg("pass");
          wifis.add(newWiFi);
          configFile.close();
          configFile = SPIFFS.open("/rfid.conf", "w");
          config.printTo(configFile);
        }
      }
    }
    else if (server.arg("add")=="rfid"){
      if (server.hasArg("uid") && server.hasArg("vendor")){
        JsonArray &rfids = config["rfid"];
        int i=0, idmax=0;
        for (i=0; i< rfids.size(); i++){
          if (rfids[i]["uid"] == server.arg("uid").toInt()) break;
          idmax = (rfids[i]["id"] < idmax)? idmax:rfids[i]["id"];
        }
        if (i==rfids.size()){
          JsonObject &newRFID = jsonBuffer.createObject();
          newRFID["id"] = idmax+1;
          newRFID["uid"] = server.arg("uid").toInt();
          newRFID["vendor"] = server.arg("vendor").toInt();
          newRFID["name"] = server.hasArg("name") ? server.arg("name"):"";
          rfids.add(newRFID);
          configFile.close();
          configFile = SPIFFS.open("/rfid.conf", "w");
          config.printTo(configFile);
        }
      }
    }
  }

  if (server.hasArg("remove"))
  {
    if (server.arg("remove")=="wifi"){
      if (server.hasArg("id")){
        JsonArray &wifis = config["wifi"];
        int i=0;
        for (i=0; i< wifis.size(); i++){
          if (wifis[i]["id"] == server.arg("id")){
            wifis.remove(i);
            configFile.close();
            configFile = SPIFFS.open("/rfid.conf", "w");
            config.printTo(configFile);
          }
        }
      }
    }
    else if (server.arg("remove")=="rfid"){
      if (server.hasArg("id")){
        JsonArray &rfids = config["rfid"];
        int i=0;
        for (i=0; i< rfids.size(); i++){
          if (rfids[i]["id"] == server.arg("id")){
            if (rfids[i]["id"]==active_id) { config["active_id"] = 0; active_id = 0; }
            rfids.remove(i);
            configFile.close();
            configFile = SPIFFS.open("/rfid.conf", "w");
            config.printTo(configFile);
          }
        }
      }
    }
  }
  
  webResponse = "";
  config["battery"] = ESP.getVcc()/901.515;
  config.prettyPrintTo(webResponse);
  configFile.close();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", webResponse.c_str());
}

void handleReboot() {
  webResponse="Rebooting...";
  server.send(200, "text/html", webResponse.c_str());
  delay(5000);
  SPIFFS.end();
  ESP.reset();
  delay(1000);
}

void handleFormat() {
  webResponse="Formatting! All data will be lost!";
  server.send(200, "text/html", webResponse.c_str());
  delay(5000);
  SPIFFS.end();
  SPIFFS.format();
  ESP.reset();
  delay(1000);
}

void setup() {
  EEPROM.begin(1);
  state = EEPROM.read(0) & 1;
  
  if (ESP.getResetReason() == "Deep-Sleep Wake" | ESP.getResetReason() == "External System") {
    state = !state;
    EEPROM.write(0, state);
    EEPROM.commit();
  }

  if (!state) {
    ESP.deepSleep(0);
  }
  EEPROM.end();
  
  
  pinMode(2, INPUT);
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  digitalWrite(0, LOW);
  digitalWrite (1, LOW);

  WiFi.disconnect(true); //true = WiFi.mode(WIFI_OFF);
  delay(100);
  
  if (DEBUGMODE) Serial.begin(115200);


  WiFi.persistent(false); //Do NOT write to flash;
  WiFi.mode(WIFI_STA);
  
  SPIFFS.begin();
  if (SPIFFS.exists("/rfid.conf")){
    File configFile = SPIFFS.open("/rfid.conf", "r");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &config = jsonBuffer.parseObject(configFile);

    DEBUGMODE = config["debug"];
    active_id = config["active_id"];

    for (int i=0; i<config["wifi"].size(); i++) {
      WiFiMulti.addAP(config["wifi"][i]["ssid"], config["wifi"][i]["pass"]);
    }

    for (int i=0; active_id && i<config["rfid"].size(); i++) {
      if (active_id == config["rfid"][i]["id"]) {
        vendor_active = config["rfid"][i]["vendor"];
        ID_active = config["rfid"][i]["uid"];
      }
    }
    
    
  }
  else {
    File configFile = SPIFFS.open("/rfid.conf", "w");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &config = jsonBuffer.createObject();
    config["debug"] = DEBUGMODE;
    config["active_id"] = 0;
    JsonArray &rfconfig = config.createNestedArray("rfid");
    JsonArray &wificonfig = config.createNestedArray("wifi");
    config.printTo(configFile);
    configFile.close();
  }

  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.on("/wifi", handleWifi);
  server.on("/rfid", handleRfid);
  server.on("/reboot", handleReboot);
  server.on("/format", handleFormat);
    
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    if (DEBUGMODE) Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    if (DEBUGMODE) Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (DEBUGMODE) Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (DEBUGMODE){
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    }
  });

//  MDNS.begin("rfid");
//  MDNS.addService("http", "tcp", 80);
//  MDNS.update();

  prepare_data(ID_active, vendor_active);

  if (DEBUGMODE) Serial.println("\r\nBoot complete!!!1!");

}

void loop() {

  if (delayed_setup) {
      if(WiFiMulti.run() == WL_CONNECTED){
        server.begin();
        ArduinoOTA.begin();
        delayed_setup=0;
        if (DEBUGMODE) Serial.printf("WiFi connected, IP: %s\r\n", WiFi.localIP().toString().c_str());
      }
      else if (millis()>15000){
        if (DEBUGMODE) Serial.println("WiFi failed, Hotspot up...");
        WiFi.scanDelete();
        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        WiFi.softAP("RFID");
        server.begin();
        ArduinoOTA.begin();
        delayed_setup=0;
      }
  }
  else {
    ArduinoOTA.handle();
    server.handleClient();
  }
  delay(2);
  
  digitalWrite (1, LOW);
  
  int i=0, j=0;
  //Manchester
  if (active_id){
    for (i=0; i<5; i++){
      for (j=0; j<64; j++){
        data[j]? pinMode(2, OUTPUT):pinMode(2, INPUT);
//        data[j]? (GPE |= 0b00000100):(GPE &= ~0b00100100);
        delayMicroseconds(256);
  
        data[j]? pinMode(2, INPUT):pinMode(2, OUTPUT);
//        data[j]? (GPE &= ~0b00000100):(GPE |= 0b00000100);
        delayMicroseconds(256);
      }
    }
    digitalWrite (1, HIGH);
  }

  

  

/*
  //PSK - never tested
  for (;;){
    for (j=0; j<64; j++) {
      pinMode(D1, OUTPUT);
//      data[j]? (GPE |= 0b00100000):(GPE &= ~0b00100000);
      delayMicroseconds(250);

      pinMode(D1, INPUT);
      data[j] ? delayMicroseconds(250):delayMicroseconds(128);
//      data[j]? (GPE &= ~0b00100000):(GPE |= 0b00100000);
    }
        delay(0);

  }
*/
  
}
