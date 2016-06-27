#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "...";
const char* password = "...";
const char* mqtt_server = "192.168.1.190"; //Сервер MQTT

IPAddress ip(192,168,1,242); //static IP
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WiFiClient espClient;
PubSubClient client(espClient);
long prev = 0;
char msg[50];
char incomingBytes[37];

#define id_connect "myhome-Conditioner"
#define len_b 37

#define b_cur_tmp 13 //Текущая температура
#define b_mode 23 //04 - DRY, 01 - cool, 02 - heat, 00 - smart 03 - вентиляция
#define b_fan_spd 25 //Скорость 02 - min, 01 - mid, 00 - max, 03 - auto
#define b_swing 27 //01 - верхний и нижний предел вкл. 00 - выкл. 02 - левый/правый вкл. 03 - оба вкл
#define b_lock_rem 28 //80 блокировка вкл. 00 -  выкл
#define b_power 29 //on/off 01 - on, 00 - off (10, 11)-Компрессор??? 09 - QUIET
#define b_fresh 31 //fresh 00 - off, 01 - on
#define b_set_tmp 35 //Установленная температура

int incomingByte = 0;
int cur_tmp;
int set_tmp;
int fan_spd;
int Mode;
//FF FF 0A 00 00 00 00 00 01 01 4D 01 5A
byte qstn[] = {10,0,0,0,0,0,1,1,77,1}; //Команда запроса
byte start[] = {255,255};
byte data[36] = {}; //Массив данных
byte on[]   = {10,0,0,0,0,0,1,1,77,2};  // Включение кондиционера
byte off[]  = {10,0,0,0,0,0,1,1,77,3}; // Выключение кондиционера
byte lock[] = {10,0,0,0,0,0,1,3,0,0}; //Блокировка пульта
byte buf[10];

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(id_connect)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("myhome/Conditioner/connection", "true");

      // ... and resubscribe
      client.subscribe("myhome/Conditioner/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
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
    set_tmp = data[b_set_tmp]+16;
    cur_tmp = data[b_cur_tmp]+16;
    Mode = data[b_mode];
    
  char b[5]; 
  String char_set_tmp = String(set_tmp);
  char_set_tmp.toCharArray(b,5); 
  client.publish("myhome/Conditioner/Set_Temp", b);
  
  String char_cur_tmp = String(cur_tmp);
  char_cur_tmp.toCharArray(b,5); 
  client.publish("myhome/Conditioner/Current_Temp", b);
  
//b_mode 23 //04 - DRY, 01 - cool, 02 - heat, 00 - smart 03 - вентиляция 
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

}

void SendData(byte req[], size_t size){
  Serial.write(start, 2);
  Serial.write(req, size);
  Serial.write(getCRC(req, size));
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  Serial.print(topic);
  Serial.print("=");
    String strTopic = String(topic);
    String strPayload = String((char*)payload);
  Serial.println(strPayload);
  //callback_iobroker(strTopic, strPayload);
  if (strTopic == "myhome/Conditioner/Set_Temp"){
    if (strPayload == "16"){
      data[b_set_tmp] = 0x00;
    }
    if (strPayload == "17"){
      data[b_set_tmp] = 0x01;
    }
    if (strPayload == "18"){
      data[b_set_tmp] = 0x02;
    }
    //data[b_set_tmp] = strPayload.getBytes(buf, 10);
    SendData(data, sizeof(data)/sizeof(byte));
  }

}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if(Serial.available() >= len_b){
    for(int i = 0; i < len_b; i++){
      data[i] = Serial.read();
    }
    while(Serial.available()){
      delay(2);
      Serial.read();
    }
    //Serial.write(data, sizeof(data)/sizeof(byte));
    InsertData(data, sizeof(data)/sizeof(byte));
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
