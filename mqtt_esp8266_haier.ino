#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "...";
const char* password = "...";
const char* mqtt_server = "192.168.1.190"; //Сервер MQTT

IPAddress ip(192,168,1,242);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WiFiClient espClient;
PubSubClient client(espClient);

long prev = 0;


#define ID_CONNECT "myhome-Conditioner"
#define LED     13
#define LEN_B   37

#define B_CUR_TMP   13  //Текущая температура
#define B_MODE      23  //04 - DRY, 01 - cool, 02 - heat, 00 - smart 03 - вентиляция
#define B_FAN_SPD   25  //Скорость 02 - min, 01 - mid, 00 - max, 03 - auto
#define B_SWING     27  //01 - верхний и нижний предел вкл. 00 - выкл. 02 - левый/правый вкл. 03 - оба вкл
#define B_LOCK_REM  28  //80 блокировка вкл. 00 -  выкл
#define B_POWER     29  //on/off 01 - on, 00 - off (10, 11)-Компрессор??? 09 - QUIET
#define B_FRESH     31  //fresh 00 - off, 01 - on
#define B_SET_TMP   35  //Установленная температура

int fresh;
int power;
int swing;
int lock_rem;
int cur_tmp;
int set_tmp;
int fan_spd;
int Mode;

byte inCheck;
byte qstn[] = {10,0,0,0,0,0,1,1,77,1}; // Команда опроса
byte start[] = {255,255};
byte data[36] = {}; //Массив данных
byte on[]   = {10,0,0,0,0,0,1,1,77,2}; // Включение кондиционера
byte off[]  = {10,0,0,0,0,0,1,1,77,3}; // Выключение кондиционера
byte lock[] = {10,0,0,0,0,0,1,3,0,0};  // Блокировка пульта
//byte buf[10];

void setup_wifi() {
  delay(10);
  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
    digitalWrite(LED, !digitalRead(LED));
  }
  //Serial.println("");
  //Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());
  digitalWrite(LED, HIGH);
}

void reconnect() {
  digitalWrite(LED, !digitalRead(LED));
  while (!client.connected()) {
    if (client.connect(ID_CONNECT)) {
      client.publish("myhome/Conditioner/connection", "true");
      client.subscribe("myhome/Conditioner/#");
      digitalWrite(LED, HIGH);
    } else {
      delay(5000);
    }
  }
}

byte getCRC(byte req[], size_t size){
  byte crc = 0;
  for (int i=0; i<size; i++){
      crc += req[i];
  }
  return crc;
}

void InsertData(byte data[], size_t size){
    set_tmp = data[B_SET_TMP]+16;
    cur_tmp = data[B_CUR_TMP]+16;
    Mode = data[B_MODE];
    fan_spd = data[B_FAN_SPD];
    swing = data[B_SWING];
    power = data[B_POWER];
    lock_rem = data[B_LOCK_REM];
    fresh = data[B_FRESH];
  /////////////////////////////////
  if (fresh == 0x00){
      client.publish("myhome/Conditioner/Fresh", "off");
  }
  if (fresh == 0x01){
      client.publish("myhome/Conditioner/Fresh", "on");
  }
  /////////////////////////////////
  if (lock_rem == 0x80){
      client.publish("myhome/Conditioner/Lock_Remote", "true");
  }
  if (lock_rem == 0x00){
      client.publish("myhome/Conditioner/Lock_Remote", "false");
  }
  /////////////////////////////////
  if (power == 0x01 || power == 0x11){
      client.publish("myhome/Conditioner/Power", "on");
  }
  if (power == 0x00 || power == 0x10){
      client.publish("myhome/Conditioner/Power", "off");
  }
  if (power == 0x09){
      client.publish("myhome/Conditioner/Power", "quiet");
  }
  if (power == 0x11 || power == 0x10){
      client.publish("myhome/Conditioner/Compressor", "on");
  } else {
    client.publish("myhome/Conditioner/Compressor", "off");
  }
  /////////////////////////////////
  if (swing == 0x00){
      client.publish("myhome/Conditioner/Swing", "off");
  }
  if (swing == 0x01){
      client.publish("myhome/Conditioner/Swing", "ud");
  }
  if (swing == 0x02){
      client.publish("myhome/Conditioner/Swing", "lr");
  }
  if (swing == 0x03){
      client.publish("myhome/Conditioner/Swing", "all");
  }
  /////////////////////////////////  
  if (fan_spd == 0x00){
      client.publish("myhome/Conditioner/Fan_Speed", "max");
  }
  if (fan_spd == 0x01){
      client.publish("myhome/Conditioner/Fan_Speed", "mid");
  }
  if (fan_spd == 0x02){
      client.publish("myhome/Conditioner/Fan_Speed", "min");
  }
  if (fan_spd == 0x03){
      client.publish("myhome/Conditioner/Fan_Speed", "auto");
  }
  /////////////////////////////////
  char b[5]; 
  String char_set_tmp = String(set_tmp);
  char_set_tmp.toCharArray(b,5);
  client.publish("myhome/Conditioner/Set_Temp", b);
  ////////////////////////////////////
  String char_cur_tmp = String(cur_tmp);
  char_cur_tmp.toCharArray(b,5);
  client.publish("myhome/Conditioner/Current_Temp", b);
  ////////////////////////////////////
  if (Mode == 0x00){
      client.publish("myhome/Conditioner/Mode", "smart");
  }
  if (Mode == 0x01){
      client.publish("myhome/Conditioner/Mode", "cool");
  }
  if (Mode == 0x02){
      client.publish("myhome/Conditioner/Mode", "heat");
  }
  if (Mode == 0x03){
      client.publish("myhome/Conditioner/Mode", "vent");
  }
  if (Mode == 0x04){
      client.publish("myhome/Conditioner/Mode", "dry");
  }
///////////////////////////////////
}

void SendData(byte req[], size_t size){
  Serial.write(start, 2);
  Serial.write(req, size);
  Serial.write(getCRC(req, size));
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  //Serial.print(topic);
  //Serial.print("=");
  String strTopic = String(topic);
  String strPayload = String((char*)payload);
  //Serial.println(strPayload);
  ///////////
  if (strTopic == "myhome/Conditioner/Set_Temp"){
    set_tmp = strPayload.toInt()-16;
    if (set_tmp >0 && set_tmp < 30){
      data[B_SET_TMP] = set_tmp;      
    }
  }
  //////////
  if (strTopic == "myhome/Conditioner/Mode"){
     if (strPayload == "smart"){
      data[B_MODE] = 0; 
    }
    if (strPayload == "cool"){
        data[B_MODE] = 1;
    }
    if (strPayload == "heat"){
        data[B_MODE] = 2; 
    }
    if (strPayload == "vent"){
        data[B_MODE] = 3; 
    }
    if (strPayload == "dry"){
        data[B_MODE] = 4; 
    }
  }
  //////////
  if (strTopic == "myhome/Conditioner/Fan_Speed"){
     if (strPayload == "max"){
      data[B_FAN_SPD] = 0; 
    }
    if (strPayload == "mid"){
        data[B_FAN_SPD] = 1;
    }
    if (strPayload == "min"){
        data[B_FAN_SPD] = 2; 
    }
    if (strPayload == "auto"){
        data[B_FAN_SPD] = 3; 
    }
  }
  ////////
  if (strTopic == "myhome/Conditioner/Swing"){
     if (strPayload == "off"){
      data[B_SWING] = 0; 
    }
    if (strPayload == "ud"){
        data[B_SWING] = 1;
    }
    if (strPayload == "lr"){
        data[B_SWING] = 2; 
    }
    if (strPayload == "all"){
        data[B_SWING] = 3; 
    }
  }
  ////////
  if (strTopic == "myhome/Conditioner/Lock_Remote"){
     if (strPayload == "true"){
      data[B_LOCK_REM] = 80;
    }
    if (strPayload == "false"){
        data[B_LOCK_REM] = 0;
    }
  }
  ////////
  if (strTopic == "myhome/Conditioner/Power"){
     if (strPayload == "off" || strPayload == "false" || strPayload == "0"){
      SendData(off, sizeof(off)/sizeof(byte));
      return;
    }
    if (strPayload == "on" || strPayload == "true" || strPayload == "1"){
      SendData(on, sizeof(on)/sizeof(byte));
      return;
    }
    if (strPayload == "quiet"){
        data[B_POWER] = 9;
    }
  }
  ////////

  //data[B_SET_TMP] = strPayload.getBytes(buf, 10);
  SendData(data, sizeof(data)/sizeof(byte));
}

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if(Serial.available() >= LEN_B){
    for(int i = 0; i < LEN_B; i++){
      data[i] = Serial.read();
    }
    while(Serial.available()){
      delay(2);
      Serial.read();
    }
    if (inCheck != data[36]){
      inCheck = data[36];
      //Serial.write(data, sizeof(data)/sizeof(byte));
      InsertData(data, sizeof(data)/sizeof(byte));
    }
  }
  if (!client.connected()){
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - prev > 2000) {
    prev = now;
    SendData(qstn, sizeof(qstn)/sizeof(byte)); //Опрос кондиционера
  }
}
