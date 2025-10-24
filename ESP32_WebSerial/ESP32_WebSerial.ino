#include <WiFi.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PI_RX_PIN 20
#define PI_TX_PIN 9

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

const char* ap_ssid = "ESP32-Serial-Config";
DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences preferences;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int numClients = 0;
bool clientConnected = false;
unsigned long lastOledUpdate = 0;
unsigned long startTime = 0;
bool apMode = false;
String currentSSID = "";
String currentIP = "";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Serial Shell</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/xterm@5.3.0/css/xterm.css" />
    <style>
        body { 
            font-family: 'Courier New', monospace; 
            background-color: #1e1e1e;
            color: #d4d4d4; 
            margin: 0; 
            overflow: hidden;
        }
        #terminal-container {
            width: 100vw;
            height: 100vh;
        }
    </style>
</head>
<body>
    <div id="terminal-container"></div>

    <script src="https://cdn.jsdelivr.net/npm/xterm@5.3.0/lib/xterm.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/xterm-addon-fit@0.8.0/lib/xterm-addon-fit.js"></script>
	<script type="module">import * as gytxxtermLocalEcho from 'https://esm.run/@gytx/xterm-local-echo'; window.gytxxtermLocalEcho=gytxxtermLocalEcho;</script>	
    <script>
        window.onload = () => {
            const termContainer = document.getElementById('terminal-container');
            
            const term = new Terminal({
                cursorBlink: true,
                theme: {
                    background: '#1e1e1e',
                    foreground: '#d4d4d4'
                },
                rows: 30,
                cols: 80
            });

            const fitAddon = new FitAddon.FitAddon();
            term.loadAddon(fitAddon);

            const localEchoAddon = new gytxxtermLocalEcho.LocalEchoAddon();
            term.loadAddon(localEchoAddon);

            term.open(termContainer);
            term.focus();
            
            function resizeTerm() {
                fitAddon.fit();
            }
            
            resizeTerm();
            window.addEventListener('resize', resizeTerm);
            
            const ws = new WebSocket(`ws://${window.location.host}/ws`);
            
            ws.binaryType = "arraybuffer";

            ws.onopen = () => { term.write('[WebSocket Connected]\r\n'); };
            ws.onclose = () => { term.write('\r\n[WebSocket Disconnected]'); };
            ws.onerror = () => { term.write('\r\n[WebSocket Error]'); };

            ws.onmessage = (event) => {
                term.write(new Uint8Array(event.data));
            };

            term.onData((data) => {
                ws.send(data);
            });
        }
    </script>
</body>
</html>
)rawliteral";

const char config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>WiFi Config</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; background: #f0f0f0; text-align: center; }
        div { background: #fff; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); 
              padding: 20px; max-width: 400px; margin: 40px auto; }
        h2 { color: #333; }
        input, select { width: 90%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 4px; }
        button { background: #007bff; color: #fff; border: none; padding: 12px 20px; 
                 border-radius: 4px; cursor: pointer; font-size: 16px; }
        button:hover { background: #0056b3; }
        #scan { background: #6c757d; }
        #scan:hover { background: #5a6268; }
    </style>
</head>
<body>
    <div>
        <h2>ESP32 WiFi Config</h2>
        <form action="/save" method="POST">
            <select id="ssid" name="ssid">
                <option value="">Select WiFi or type below</option>
            </select>
            <input type="text" name="ssid_manual" placeholder="Or enter SSID manually">
            <input type="password" name="password" placeholder="Password">
            <button type="submit">Save & Reboot</button>
        </form>
        <button id="scan" onclick="scanNetworks()">Scan for WiFi</button>
    </div>
    <script>
        function scanNetworks() {
            const select = document.getElementById('ssid');
            select.innerHTML = '<option value="">Scanning...</option>';
            fetch('/scan')
                .then(response => response.json())
                .then(networks => {
                    select.innerHTML = '<option value="">Select WiFi or type below</option>';
                    networks.forEach(net => {
                        select.innerHTML += `<option value="${net.ssid}">${net.ssid} (${net.rssi})</option>`;
                    });
                })
                .catch(err => {
                    select.innerHTML = '<option value="">Scan failed</option>';
                    console.error(err);
                });
        }
        window.onload = scanNetworks;
    </script>
</body>
</html>
)rawliteral";

String escapeJson(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  return s;
}

uint32_t parseSerialConfig(String config) {
    if (config == "8N1") return SERIAL_8N1;
    if (config == "7E1") return SERIAL_7E1;
    if (config == "8N2") return SERIAL_8N2;
    return SERIAL_8N1;
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            numClients++;
            clientConnected = true;
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            numClients--;
            if (numClients == 0) {
                clientConnected = false;
            }
            break;
        case WS_EVT_DATA: { 
            String msg = String((char*)data, len);

            if (msg.startsWith("CMD::BAUD SET ")) {
                String args = msg.substring(14);
                args.trim();
                
                int firstSpace = args.indexOf(' ');
                long baud = 115200;
                uint32_t config = SERIAL_8N1;
                String configStr = "8N1";

                if (firstSpace != -1) {
                    baud = args.substring(0, firstSpace).toInt();
                    configStr = args.substring(firstSpace + 1);
                    configStr.trim();
                    config = parseSerialConfig(configStr);
                } else {
                    baud = args.toInt();
                }

                if (baud > 0) {
                    Serial1.end();
                    Serial1.begin(baud, config, PI_RX_PIN, PI_TX_PIN); 
                    String response = "[Bridge] Serial1 reconfigured to " + String(baud) + ", " + configStr + "\n";
                    Serial.print(response);
                    client->text(response);
                } else {
                    client->text("[Bridge] Error: Invalid baud rate.\n");
                }
            } else {
                Serial1.print(msg);
                Serial1.flush();
            }
            break;
        }
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void setupOLED() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ESP32 Serial Bridge");
    display.println("Initializing...");
    display.display();
    delay(1000);
}

void updateOLED() {
    if (clientConnected) {
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(15, 20);
        display.println("CLIENT");
        display.println("CONNECTED");
        display.display();
        return;
    }

    if (millis() - lastOledUpdate > 1000) {
        lastOledUpdate = millis();
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);

        if (apMode) {
            display.println("--- AP MODE ---");
            display.println("SSID:");
            display.setTextSize(2);
            display.println(ap_ssid);
            display.setTextSize(1);
            display.println("IP: 192.168.4.1");
        } else {
            display.println("--- STA MODE ---");
            display.println(currentIP);
            display.println(currentSSID);
            display.printf("Clients: %d\n", numClients);
            
            unsigned long s = (millis() - startTime) / 1000;
            unsigned long m = s / 60;
            unsigned long h = m / 60;
            display.printf("Uptime: %02lu:%02lu:%02lu", h % 24, m % 60, s % 60);
        }
        
        display.display();
    }
}

void setupAP() {
    Serial.println("Setting up AP mode...");
    apMode = true;
    WiFi.softAP(ap_ssid);
    dnsServer.start(53, "*", WiFi.softAPIP());

    currentIP = WiFi.softAPIP().toString();
    Serial.print("AP IP address: ");
    Serial.println(currentIP);

    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "[";
        int n = WiFi.scanNetworks();
        Serial.printf("%d networks found\n", n);
        for (int i = 0; i < n; ++i) {
            json += "{";
            json += "\"ssid\":\"" + escapeJson(WiFi.SSID(i)) + "\"";
            json += ",\"rssi\":" + String(WiFi.RSSI(i));
            json += "}";
            if (i < n - 1) json += ",";
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        String new_ssid = request->arg("ssid");
        if (request->hasArg("ssid_manual") && request->arg("ssid_manual") != "") {
            new_ssid = request->arg("ssid_manual");
        }
        String new_pass = request->arg("password");

        Serial.printf("Saving new credentials. SSID: %s\n", new_ssid.c_str());

        preferences.begin("wifi-creds", false);
        preferences.putString("ssid", new_ssid);
        preferences.putString("password", new_pass);
        preferences.end();

        String resp = "<html><body><h2>Credentials Saved!</h2><p>Rebooting...<script>setTimeout(()=>window.location.href='/', 3000);</script></body></html>";
        request->send(200, "text/html", resp);
        
        delay(1000);
        ESP.restart();
    });
    
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(200, "text/html", config_html);
    });

    server.begin();
}

void setupSTA() {
    Serial.println("Setting up STA mode...");
    apMode = false;
    currentSSID = WiFi.SSID();
    currentIP = WiFi.localIP().toString();
    Serial.print("STA IP address: ");
    Serial.println(currentIP);

    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("\nBooting...");

    Serial1.begin(115200, SERIAL_8N1, PI_RX_PIN, PI_TX_PIN);

    Wire.begin();
    setupOLED();
    startTime = millis();

    preferences.begin("wifi-creds", true);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("password", "");
    preferences.end();

    if (ssid == "") {
        Serial.println("No credentials found.");
        setupAP();
    } else {
        Serial.print("Connecting to: ");
        Serial.println(ssid);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Connecting to:");
        display.println(ssid);
        display.display();

        WiFi.begin(ssid.c_str(), pass.c_str());
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\nFailed to connect.");
            WiFi.disconnect(true);
            setupAP();
        } else {
            Serial.println("\nConnected!");
            setupSTA();
        }
    }
}

void loop() {
    if (apMode) {
        dnsServer.processNextRequest();
    }

    size_t len = Serial1.available();
    if (len) {
        uint8_t buf[len];
        Serial1.readBytes(buf, len);
        
        Serial.write(buf, len); 

        if (numClients > 0) {
            ws.binaryAll(buf, len); 
        }
    }

    updateOLED();
    ws.cleanupClients();
}
