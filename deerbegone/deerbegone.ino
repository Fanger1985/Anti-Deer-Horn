#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Define network credentials
const char* ssid = "Deer_Begone"; // WiFi network name, no password needed

// Define GPIO pin numbers
const int pirPin = 13; // PIR sensor input pin
const int relayPin = 12; // Relay control pin

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
      display: flex; 
      flex-direction: column; 
      justify-content: center; 
      align-items: center; 
      height: 100vh; 
      margin: 0; 
    }
    .container { 
      background: rgba(255, 255, 255, 0.1); 
      border-radius: 20px; 
      box-shadow: 10px 10px 30px rgba(0, 0, 0, 0.5), -10px -10px 30px rgba(255, 255, 255, 0.2); 
      padding: 50px; 
      text-align: center; 
      backdrop-filter: blur(10px); 
    }
    h1 { 
      color: #ff0000; 
      margin-bottom: 40px; 
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
  </style>
</head>
<body>
  <div class="container">
    <h1>EFFING DEER BEGONE!</h1>
    <button onclick="controlAlarm('on')">Activate Alarm</button>
    <button onclick="controlAlarm('off')">Deactivate Alarm</button>
    <button id="honkButton" onclick="honkNow()">HONK NOW</button>
  </div>
  <script>
    function controlAlarm(state) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/alarm?state=" + state, true);
      xhr.send();
    }
    function honkNow() {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/honk", true);
      xhr.send();
    }
  </script>
</body>
</html>
)rawliteral";

bool alarmActivated = false; // State of the alarm system

void setup() {
  // Initialize the Serial Monitor for debugging
  Serial.begin(115200);

  // Set GPIO pin modes
  pinMode(pirPin, INPUT); // Set PIR sensor pin as input
  pinMode(relayPin, OUTPUT); // Set relay pin as output
  digitalWrite(relayPin, LOW); // Ensure relay is off initially

  // Initialize WiFi Access Point
  WiFi.softAP(ssid);

  // Output the IP address of the ESP32 Access Point to the Serial Monitor
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

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
        alarmActivated = true; // Activate the alarm system
        Serial.println("Alarm Activated");
      } else {
        alarmActivated = false; // Deactivate the alarm system
        digitalWrite(relayPin, LOW); // Ensure the horn is off when deactivating
        Serial.println("Alarm Deactivated");
      }
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Bad Request");
      Serial.println("Error: Bad Request - Alarm");
    }
  });

  // Handle requests to honk the horn immediately
  server.on("/honk", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(relayPin, HIGH); // Turn horn on
    delay(1000); // Keep the horn on for 1 second
    digitalWrite(relayPin, LOW); // Turn horn off
    request->send(200, "text/plain", "HONKED");
    Serial.println("Horn Honked");
  });

  // Start the server
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Check the PIR sensor for motion only if the alarm is activated
  if (alarmActivated && digitalRead(pirPin) == HIGH) {
    Serial.println("Motion detected - Horn Activated");
    digitalWrite(relayPin, HIGH); // Turn horn on if motion is detected
    delay(5000); // Keep the horn on for 5 seconds
    digitalWrite(relayPin, LOW); // Turn horn off
    Serial.println("Horn Deactivated");
  }
}
