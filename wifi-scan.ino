#include "WiFi.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT broker credentials
const char* mqtt_server = "100.42.181.66";
const int mqtt_port = 1883;
const char* mqtt_topic = "piano";

WiFiClient espClient;
PubSubClient client(espClient);

// Buzzer pin
const int buzzerPin = 16;

// Volume Control
int volume = 100; // Default volume (0 - 100)

// Define frequencies for musical notes (in Hz)
const int do_freq = 262;
const int do_sharp_freq = 277;
const int re_freq = 294;
const int re_sharp_freq = 311;
const int mi_freq = 330;
const int fa_freq = 349;
const int fa_sharp_freq = 370;
const int so_freq = 392;
const int so_sharp_freq = 415;
const int la_freq = 440;
const int la_sharp_freq = 466;
const int si_freq = 494;

// Function to connect to WiFi
void setup_wifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to reconnect to MQTT broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Function to play a note with volume
void playNoteWithDuration(const String& note, int duration) {
  int frequency = 0;

  // Match the note with its frequency
  if (note == "do") frequency = do_freq;
  else if (note == "do#") frequency = do_sharp_freq;
  else if (note == "re") frequency = re_freq;
  else if (note == "re#") frequency = re_sharp_freq;
  else if (note == "mi") frequency = mi_freq;
  else if (note == "fa") frequency = fa_freq;
  else if (note == "fa#") frequency = fa_sharp_freq;
  else if (note == "so") frequency = so_freq;
  else if (note == "so#") frequency = so_sharp_freq;
  else if (note == "la") frequency = la_freq;
  else if (note == "la#") frequency = la_sharp_freq;
  else if (note == "si") frequency = si_freq;

  if (frequency > 0) {
    int adjustedFrequency = map(volume, 0, 100, 0, frequency); // Adjust volume with frequency
    tone(buzzerPin, adjustedFrequency, duration);
    delay(duration);
    noTone(buzzerPin);
  } else {
    Serial.println("Invalid note received: " + note);
  }
}

// Function to set volume
void setVolume(int newVolume) {
  volume = constrain(newVolume, 0, 100); // Ensure volume stays between 0 and 100
  Serial.print("Volume set to: ");
  Serial.println(volume);
}

// MQTT callback function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Parse JSON payload
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }

  // Handle Volume Change
  if (doc.containsKey("volume")) {
    int newVolume = doc["volume"];
    setVolume(newVolume);
    return;
  }

  // Handle Note Playback
  String note = doc["note"];
  int duration = doc["duration"];

  Serial.print("Playing note: ");
  Serial.print(note);
  Serial.print(" for ");
  Serial.print(duration);
  Serial.println(" ms");

  playNoteWithDuration(note, duration);
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(100);
}
