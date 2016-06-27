#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "........";
const char* password = "........";
const char* mqtt_server = "192.168.1.190"; //Сервер MQTT

IPAddress ip(192,168,1,242); //static IP
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WiFiClient espClient;
PubSubClient client(espClient);
long prev = 0;
char msg[50];

#define id_connect "myhome-Conditioner"
#define len_b 33

#define cur_tmp 3 //Номер байта текущей температуры
#define set_tmp 3 //Байт установленной температуры
#define fan_spd 3 //Скорость вентилятора


int incomingByte = 0;
int cur_temp;
//FF FF 0A 00 00 00 00 00 01 01 4D 01 5A
byte qstn[] = {10,0,0,0,0,0,1,1,77,1}; //Команда запроса
byte start[] = {255,255};
byte data[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //Массив данных
byte on[] = {10,0,0,0,0,0,1,1,77,1}; //// Включение кондиционера
byte off[] = {10,0,0,0,0,0,1,1,77,1}; //// Выключение кондиционера

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

int Length(arr){
  return sizeof(arr)/sizeof(byte);
}

byte getCRC(byte req[]){
  byte crc = 0;
  for (int i=0; i<Length(req[]); i++){
    //if (req[i] !== 0){
      crc += req[i];
    //}
  }
  return crc;
}

void InsertData(data){
  cur_temp = data[cur_tmp];
  
  switch (cur_temp) {
    case 1:
      cur_temp = 16;
      break;
    case 2:
      cur_temp = 17;
      break;
    default:
  }
  client.publish("myhome/Conditioner/Current_Temp", cur_temp);
}

void SendData(byte req[]){
  Serial.write(start, 2);
  Serial.write(req, Length(req));
  Serial.write(getCRC(req));
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
    switch (strPayload) {
    case 16:
      strPayload = 1;
      break;
    case 17:
      strPayload = 2;
      break;
    default:
  }
    data[set_tmp] = strPayload;
    SendData(data);
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
    for(byte i = 0; i < len_b; i++){
      data[i] = Serial.read();
    }
    while(Serial.available()){
      delay(2);
      Serial.read();
    }
    Serial.write(data, Length(data));
  }
  if (!client.connected()){
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - prev > 2000) {
    prev = now;
    SendData(qstn); //Опрос кондиционера
  }
}
