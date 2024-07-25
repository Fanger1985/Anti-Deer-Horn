#include <esp_now.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// GPIO pin for the relay
const int relayPin = 12;

// MAC address of the PIR Sensor Unit
uint8_t pirSensorAddress[] = {0xD4, 0x8A, 0xFC, 0xC6, 0x65, 0x5C};

// Create an instance of the server
AsyncWebServer server(80);

// HTML and CSS for the control panel
const char controlPanelHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>EFFING DEER BEGONE!</title>
  <style>
    body { 
      font-family: Arial, sans-serif; 
      background: #121212; 
      color: white;
      display: flex; 
      flex-direction: column; 
      justify-content: center; 
      align-items: center; 
      height: 100vh; 
      margin: 0; 
    }
    .container { 
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      background: rgba(255, 255, 255, 0.1); 
      border-radius: 20px; 
      box-shadow: 10px 10px 30px rgba(0, 0, 0, 0.5), -10px -10px 30px rgba(255, 255, 255, 0.2); 
      padding: 20px; 
      text-align: center; 
      backdrop-filter: blur(10px); 
      width: 90%; 
      max-width: 500px;
    }
    h1 { 
      color: #ff0000; 
      margin-bottom: 20px; 
      text-shadow: 3px 3px 5px rgba(0, 0, 0, 0.7); 
    }
    button { 
      padding: 15px 25px; 
      font-size: 24px; 
      margin: 10px; 
      border: none; 
      border-radius: 10px; 
      color: #fff; 
      background: linear-gradient(145deg, #1f1f1f, #262626); 
      box-shadow: 5px 5px 15px rgba(0, 0, 0, 0.5), -5px -5px 15px rgba(255, 255, 255, 0.1); 
      transition: all 0.3s ease; 
      cursor: pointer; 
    }
    button:active { 
      background: linear-gradient(145deg, #1a1a1a, #2b2b2b); 
      box-shadow: 2px 2px 10px rgba(0, 0, 0, 0.5), -2px -2px 10px rgba(255, 255, 255, 0.1); 
    }
    button:hover { 
      background: linear-gradient(145deg, #292929, #3a3a3a); 
    }
    #honkButton { 
      background: red; 
      font-size: 28px; 
    }
    .status {
      margin: 10px 0;
      padding: 10px;
      border-radius: 10px;
      font-size: 20px;
      font-weight: bold;
      width: 100%;
    }
    .inactive {
      background-color: #444;
      color: #bbb;
    }
    .active {
      background-color: #f00;
      color: #fff;
    }
    .warm-bodies {
      background-color: #0f0;
      color: #000;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>EFFING DEER BEGONE!</h1>
    <div id="hornStatus" class="status inactive">Horn: Inactive</div>
    <div id="bodyStatus" class="status inactive">Warm Bodies: Not Detected</div>
    <button onclick="controlAlarm('on')">Activate Alarm</button>
    <button onclick="controlAlarm('off')">Deactivate Alarm</button>
    <button id="honkButton" onclick="honkNow()">HONK NOW</button>
  </div>
  <script>
    function controlAlarm(state) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/alarm?state=" + state, true);
      xhr.send();
      if (state === 'on') {
        document.getElementById('hornStatus').classList.add('active');
        document.getElementById('hornStatus').classList.remove('inactive');
        document.getElementById('hornStatus').innerText = "Horn: Active";
      } else {
        document.getElementById('hornStatus').classList.add('inactive');
        document.getElementById('hornStatus').classList.remove('active');
        document.getElementById('hornStatus').innerText = "Horn: Inactive";
      }
    }
    function honkNow() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/honk", true);
      xhr.send();
      document.getElementById('hornStatus').classList.add('active');
      document.getElementById('hornStatus').classList.remove('inactive');
      document.getElementById('hornStatus').innerText = "Horn: Honking";
      setTimeout(() => {
        document.getElementById('hornStatus').classList.remove('active');
        document.getElementById('hornStatus').classList.add('inactive');
        document.getElementById('hornStatus').innerText = "Horn: Inactive";
      }, 2000);
    }

    // Function to update warm body status
    function updateBodyStatus(detected) {
      if (detected) {
        document.getElementById('bodyStatus').classList.add('warm-bodies');
        document.getElementById('bodyStatus').classList.remove('inactive');
        document.getElementById('bodyStatus').innerText = "Warm Bodies: Detected";
      } else {
        document.getElementById('bodyStatus').classList.add('inactive');
        document.getElementById('bodyStatus').classList.remove('warm-bodies');
        document.getElementById('bodyStatus').innerText = "Warm Bodies: Not Detected";
      }
    }

    // Simulated function for warm body detection
    setInterval(() => {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/bodyStatus", true);
      xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
          var response = xhr.responseText;
          updateBodyStatus(response === '1');
        }
      };
      xhr.send();
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

// Initialize alarm status
bool alarmActivated = false;
bool warmBodiesDetected = false;

void setup() {
  // Initialize the Serial Monitor for debugging
  Serial.begin(115200);

  // Set relay pin as output
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.softAP("Deer_Begone");

  // Output the IP address of the ESP32 Access Point to the Serial Monitor
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, pirSensorAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add the peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Register the receive callback
  esp_now_register_recv_cb(OnDataRecv);

  // Handle requests for the root URL ("/") by serving the control panel HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", controlPanelHTML);
    Serial.println("Control Panel Accessed");
  });

  // Handle requests to control the alarm
  server.on("/alarm", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("state")) {
      String state = request->getParam("state")->value();
      if (state == "on") {
        alarmActivated = true;
        esp_now_send(pirSensorAddress, (uint8_t *)"A", sizeof("A"));
        Serial.println("Alarm Activated");
      } else {
        alarmActivated = false;
        esp_now_send(pirSensorAddress, (uint8_t *)"D", sizeof("D"));
        Serial.println("Alarm Deactivated");
        digitalWrite(relayPin, LOW); // Ensure the horn is off when deactivating
      }
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Bad Request");
      Serial.println("Error: Bad Request - Alarm");
    }
  });

  // Handle requests to honk the horn immediately
  server.on("/honk", HTTP_GET, [](AsyncWebServerRequest *request){
    esp_now_send(pirSensorAddress, (uint8_t *)"H", sizeof("H"));
    digitalWrite(relayPin, HIGH); // Turn horn on
    delay(1000); // Keep the horn on for 1 second
    digitalWrite(relayPin, LOW); // Turn horn off
    request->send(200, "text/plain", "HONKED");
    Serial.println("Horn Honked");
  });

  // Handle requests to get the warm body detection status
  server.on("/bodyStatus", HTTP_GET, [](AsyncWebServerRequest *request){
    String status = warmBodiesDetected ? "1" : "0";
    request->send(200, "text/plain", status);
  });

  // Start the server
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // No need to put anything in the loop for now
}

// Callback function when data is received
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  char message = incomingData[0];
  
  if (message == 'M' && alarmActivated) {
    Serial.println("Motion detected by PIR unit - Horn Activated");
    warmBodiesDetected = true;
    digitalWrite(relayPin, HIGH);
    delay(5000); // Horn honk duration
    digitalWrite(relayPin, LOW);
    warmBodiesDetected = false;
  }
}
