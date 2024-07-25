#include <esp_now.h>
#include <WiFi.h>

// GPIO pin for the PIR sensor
const int pirPin = 27;

// MAC address of the Relay Unit
uint8_t relayUnitAddress[] = {0xD8, 0xBC, 0x38, 0xFA, 0x9C, 0x10};

// Alarm activation status
bool alarmActivated = false;

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);

  // Set PIR pin as input
  pinMode(pirPin, INPUT);

  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, relayUnitAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add the peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Register the receive callback
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("PIR Sensor Unit setup complete. Waiting for motion detection and commands...");
}

void loop() {
  // Check the PIR sensor for motion
  int pirState = digitalRead(pirPin);
  Serial.print("PIR sensor state: ");
  Serial.println(pirState);

  if (alarmActivated && pirState == HIGH) {
    Serial.println("Motion detected");

    // Send a signal to the relay unit
    esp_err_t result = esp_now_send(relayUnitAddress, (uint8_t *)"M", sizeof("M"));
    
    if (result == ESP_OK) {
      Serial.println("Motion signal sent to Relay Unit");
    } else {
      Serial.println("Error sending motion signal to Relay Unit");
    }

    // Delay to avoid multiple triggers in quick succession
    delay(10000); // Adjust this delay as needed
  } else if (!alarmActivated) {
    Serial.println("Alarm is not activated, no action taken.");
  }

  delay(500); // Check the PIR sensor state every 500ms
}

// Callback function when data is received
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  char message = incomingData[0];

  if (message == 'A') {
    alarmActivated = true;
    Serial.println("Alarm Activated");
  } else if (message == 'D') {
    alarmActivated = false;
    Serial.println("Alarm Deactivated");
  } else if (message == 'H') {
    Serial.println("Horn Honk Command Received");
  }
}
