//DEVICE
#include <SoftwareSerial.h>
#include <Servo.h>
#include <time.h>
#define TX_PIN D1
#define RX_PIN D2

int pinLed = D4;
int pinPir = D5;
int pinServo = D6;
int pirState = LOW;
int val = 0;
Servo myservo;

// WIFI
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#define ssid "Minh Vang"
#define password "choxincaighe"
// Thông tin về MQTT Broker
#define mqtt_server "broker.emqx.io"
const uint16_t mqtt_port = 1883;  //Port của MQTT broker
#define mqtt_topic_pub_led "finalpj/device/infoservo"
#define mqtt_topic_sub_led "finalpj/device/infoservo"

WiFiClient espClient;
PubSubClient client(espClient);
StaticJsonDocument<256> doc;  //PubSubClient limits the message size to 256 bytes (including header)
char ledstatus[32] = "on";

void setup() {
  Serial.begin(9600);
  // bluetooth.begin(9600);

  // pinMode(RX_PIN, INPUT);
  // pinMode(TX_PIN, OUTPUT);
  pinMode(pinLed, OUTPUT);
  myservo.attach(pinServo);
  pinMode(pinPir, INPUT);

  digitalWrite(pinLed, HIGH);
  myservo.write(0);
  Serial.println("Bluetooth On please press 1 or 0 blink LED");

  setup_wifi();
  // cài đặt server eclispe mosquitto / mqttx và lắng nghe client ở port 1883
  setupTime();
  client.setServer(mqtt_server, mqtt_port);
  // gọi hàm callback để thực hiện các chức năng publish/subcribe
  client.setCallback(callback);
  // gọi hàm reconnect() để thực hiện kết nối lại với server khi bị mất kết nối
  reconnect();
}

void loop() {
  // kiểm tra nếu ESP8266 chưa kết nối được thì sẽ thực hiện kết nối lại
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //Tu dong mo cua
  autoOpenDoor();
}

void autoOpenDoor() {
  val = digitalRead(pinPir);  // đọc giá trị đầu vào.
  if (val == HIGH)            // nếu giá trị ở mức cao.(1)
  {
    if (pirState == LOW) {
      Serial.println("Motion detected!");
      openDoor(true);
      digitalWrite(pinLed, HIGH);  // LED On
      pirState = HIGH;
    }
  } else {
    if (pirState == HIGH) {
      Serial.println("Motion ended!");
      digitalWrite(pinLed, LOW);
      openDoor(false);
      pirState = LOW;
    }
  }
}

void openDoor(bool isEnable) {
  size_t n;
  char buffer[256];
  if (isEnable) {
    myservo.write(180);
    if (client.connect("finalpf_finalpj")) {
      doc["datetime"] = getTime(); 
      doc["device"] = "SERVO";
      doc["message"] = "The door is openning";
      doc["status"] = "ON";      
      n = serializeJson(doc, buffer);
      client.publish(mqtt_topic_pub_led, buffer, n);
      // đăng kí nhận gói tin tại topic wemos/ledstatus
      client.subscribe(mqtt_topic_sub_led);
    }
  }
  if (!isEnable) {
    myservo.write(0);
    if (client.connect("finalpf_finalpj")) {
      doc["datetime"] = getTime(); 
      doc["device"] = "SERVO";
      doc["message"] = "The door is closing";
      doc["status"] = "OFF";  // Thêm thời gian thực
      size_t n = serializeJson(doc, buffer);
      client.publish(mqtt_topic_pub_led, buffer, n);
        // đăng kí nhận gói tin tại topic wemos/ledstatus
      client.subscribe(mqtt_topic_sub_led);
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // kết nối đến mạng Wifi
  WiFi.begin(ssid, password);
  // in ra dấu . nếu chưa kết nối được đến mạng Wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // in ra thông báo đã kết nối và địa chỉ IP của ESP8266
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupTime() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // GMT+7
  Serial.println("Synchronizing time...");
  while (time(nullptr) < 100000) { // Chờ thời gian được đồng bộ
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nTime synchronized.");
}

void callback(char* topic, byte* payload, unsigned int length) {
  //chuyen doi *byte sang json
  deserializeJson(doc, payload, length);
  //doc thong tin status tu chuỗi json trả về
  strlcpy(ledstatus, doc["status"] | "on", sizeof(ledstatus));
  String mystring(ledstatus);
  //in ra tên của topic và nội dung nhận được từ kênh MQTT lens đã publish
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  // in trang thai cua led
  Serial.println(ledstatus);
}

void reconnect() {
  // lặp cho đến khi được kết nối trở lại
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // hàm connect có đối số thứ 1 là id đại diện cho mqtt client, đối số thứ 2 là username và đối số thứ 3 là password
    if (client.connect("finalpf_finalpj")) {
      Serial.println("connected");
      // publish gói tin "Hello esp8266!" đến topic mqtt_topic_pub_test

      char buffer[256];
      doc["message"] = "Hello esp8266!";
      size_t n = serializeJson(doc, buffer);
      client.publish(mqtt_topic_pub_led, buffer, n);
    } else {
      // in ra màn hình trạng thái của client khi không kết nối được với MQTT broker
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // delay 5s trước khi thử lại
      delay(5000);
    }
  }
}

String getTime() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timeString[30];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(timeString);
}
