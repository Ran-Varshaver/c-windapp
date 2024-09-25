#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

const char* ssid = "SSID_HERE";
const char* password = "PASS_HERE";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int analogPin = 34;
const int button1Pin = 26;
const int button2Pin = 27;

String currentDay;
int dayIndex = 0;
int maxIndex = 6;
int currentHour = 1;

String timestamp;
int firstDay = 1;

float knots = 0.0;
float gusts = 0.0;
int dir = 0;

String hourArr[7][24];
float knotsArr[7][24];
float gustArr[7][24];
int dirArr[7][24];

const float pi = 3.14159265359;

float degToRad(float degrees) {
  return degrees * (PI / 180.0);
}

void arrow(int dir) {
  int baseX = SCREEN_WIDTH - 16;
  int baseY = SCREEN_HEIGHT - 16;
  int size = 10;

  dir = (dir + 180) % 360;

  int x1 = baseX;
  int y1 = baseY - size;

  int x2 = baseX - size / 2;
  int y2 = baseY + size / 2;

  int x3 = baseX + size / 2;
  int y3 = baseY + size / 2;

  float rad = degToRad(dir);
  
  int rotatedX1 = baseX + (x1 - baseX) * cos(rad) - (y1 - baseY) * sin(rad);
  int rotatedY1 = baseY + (x1 - baseX) * sin(rad) + (y1 - baseY) * cos(rad);

  int rotatedX2 = baseX + (x2 - baseX) * cos(rad) - (y2 - baseY) * sin(rad);
  int rotatedY2 = baseY + (x2 - baseX) * sin(rad) + (y2 - baseY) * cos(rad);

  int rotatedX3 = baseX + (x3 - baseX) * cos(rad) - (y3 - baseY) * sin(rad);
  int rotatedY3 = baseY + (x3 - baseX) * sin(rad) + (y3 - baseY) * cos(rad);

  display.fillTriangle(rotatedX1, rotatedY1, rotatedX2, rotatedY2, rotatedX3, rotatedY3, SSD1306_WHITE);
}

void setup() {
  Serial.begin(115200);
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
  display.setTextColor(SSD1306_WHITE); 

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.println("attempting connection");
  }
  Serial.println("connection successful");

  getWind();
}

void getWind() {
  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=32.388278772797904&longitude=34.86379265785218&hourly=wind_speed_10m,wind_direction_10m,wind_gusts_10m&wind_speed_unit=kn";

  http.begin(url);
  int httpResponse = http.GET();

  if(httpResponse > 0) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    Serial.println(payload);

    if (!error) {
      JsonArray timeArray = doc["hourly"]["time"];
      JsonArray windSpeedArray = doc["hourly"]["wind_speed_10m"];
      JsonArray windGustArray = doc["hourly"]["wind_gusts_10m"];
      JsonArray windDirArray = doc["hourly"]["wind_direction_10m"];

      currentDay = timeArray[0].as<String>().substring(5, 10);
      
      for (int i = 0; i < 168; i++) {
        int d = i / 24;
        int h = i % 24;
        hourArr[d][h] = timeArray[i].as<String>();
        knotsArr[d][h] = windSpeedArray[i].as<float>();
        gustArr[d][h] = windGustArray[i].as<float>();
        dirArr[d][h] = windDirArray[i].as<int>();
      }

    } else {
      Serial.print("failed to parse .JSON ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("HTTP error ");
    Serial.println("httpResponse");
  }
}

void loop () {
  int analogValue = analogRead(analogPin);
  int i = map(analogValue, 0, 4095, 0, 11);
  int currentHour = i * 2 +1;

  bool button1State = digitalRead(button1Pin);
  bool button2State = digitalRead(button2Pin);

  if(button1State == LOW) {
    dayIndex++;
    delay(150);
    if(dayIndex > maxIndex) {
      dayIndex = maxIndex;
    }
    currentDay = hourArr[dayIndex][0].substring(5, 10);
  }

  if(button2State == LOW) {
    dayIndex--;
    delay(150);
    if(dayIndex < 0) {
      dayIndex = 0;
    }
    currentDay = hourArr[dayIndex][0].substring(5, 10);
  }

  String month = currentDay.substring(0,2);
  String day = currentDay.substring(3,5);
  String formattedDate = day + "/" + month;

  knots = knotsArr[dayIndex][currentHour];
  gusts = gustArr[dayIndex][currentHour];
  dir = dirArr[dayIndex][currentHour];

  display.clearDisplay();
  display.setCursor(0, 0);

  display.setTextSize(1);
  display.println("Date");
  display.setTextSize(2);
  display.print(day);
  display.setTextSize(1);
  display.println("/" + month);
  display.println();

  display.setCursor(64,0);
  display.setTextSize(1);
  display.println("Hour");
  display.setCursor(64,8);
  display.setTextSize(2);
  display.print(currentHour);
  display.setTextSize(1);
  display.println(":00");

  display.setCursor(0,42);
  display.setTextSize(1);
  display.print("Wind: ");
  display.println(knots);

  display.print("Gusts: ");
  display.println(gusts);

  arrow(dir);

  display.display(); 

}