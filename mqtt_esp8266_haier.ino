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
#define DATA_LENGTH sizeof(request)/sizeof(byte)

byte request[]={62,1,7,23,174,0,191,43,117}; //Команда запроса

/**int Length(arr){
  return sizeof(arr)/sizeof(byte)
}
*/

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

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  Serial.print(topic);
  Serial.print("=");
    String strTopic = String(topic);
    String strPayload = String((char*)payload);
  Serial.println(strPayload);
  //callback_iobroker(strTopic, strPayload);

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(id_connect)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("myhome/Conditioner/connection", true);

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

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - prev > 2000) {
    prev = now;
    Serial.write(request,DATA_LENGTH); //Опрос кондиционера
  }
}
