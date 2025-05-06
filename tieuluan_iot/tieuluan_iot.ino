#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi & MQTT
const char* ssid = "BDU-Student";
const char* password = "HocHoiHieuHanh";
const char* mqtt_server = "192.168.123.106";

// Cảm biến
#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#define MQ135_PIN A0

// LED
#define LED_XANH D5
#define LED_VANG D6
#define LED_DO   D7

WiFiClient espClient;
PubSubClient client(espClient);

int che_do = 0; // 0 = chất lượng không khí, 1 = dự đoán mưa

void guiDuLieu(float t, float h, int mq) {
  char tempStr[8], humStr[8], airStr[8];
  dtostrf(t, 1, 2, tempStr);
  dtostrf(h, 1, 2, humStr);
  itoa(mq, airStr, 10);

  client.publish("nhiet_do", tempStr);
  client.publish("do_am", humStr);
  client.publish("khong_khi", airStr);
  client.publish("che_do/hien_tai", che_do == 0 ? "ON" : "OFF"); // Gửi ON/OFF
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Kết nối MQTT...");
    if (client.connect("ESP8266Client")) {
      Serial.println("OK");
      client.subscribe("che_do/dieu_khien");
    } else {
      Serial.print("Lỗi: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];

  if (String(topic) == "che_do/dieu_khien") {
    if (message == "ON") che_do = 0;
    else if (message == "OFF") che_do = 1;

    Serial.print("Chuyển chế độ thành: ");
    Serial.println(che_do == 0 ? "Không khí" : "Dự đoán mưa");
  }
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi đã kết nối");
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_XANH, OUTPUT);
  pinMode(LED_VANG, OUTPUT);
  pinMode(LED_DO, OUTPUT);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void dieuKhienLED(int xanh, int vang, int do_) {
  digitalWrite(LED_XANH, xanh);
  digitalWrite(LED_VANG, vang);
  digitalWrite(LED_DO, do_);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  static unsigned long lastTime = 0;
  if (millis() - lastTime > 10000) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int mq = analogRead(MQ135_PIN);

    if (isnan(h) || isnan(t)) {
      Serial.println("Không đọc được cảm biến DHT!");
      return;
    }

    guiDuLieu(t, h, mq);

    if (che_do == 0) {
      // Chất lượng không khí
    if (mq < 50) {dieuKhienLED(LOW, LOW, HIGH);   // Tốt
}   else if (mq >= 50 && mq < 150) {dieuKhienLED(LOW, HIGH, LOW);   // Trung bình
}   else {dieuKhienLED(HIGH, LOW, LOW);   // Ô nhiễm
}
    } else {
      // Dự đoán mưa
      if (h > 80 && t < 25)      dieuKhienLED(HIGH, LOW, LOW);  // Mưa
      else if (h > 60)           dieuKhienLED(LOW, HIGH, LOW);  // Có khả năng
      else                       dieuKhienLED(LOW, LOW, HIGH);  // Không mưa
    }

    lastTime = millis();
  }
}


