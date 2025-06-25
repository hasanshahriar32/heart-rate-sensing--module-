#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

/*
 * ESP8266 Heart Rate Monitor
 * ==========================
 * This project reads heart rate data from a pulse sensor connected to A0,
 * displays it on Serial Monitor, and hosts a beautiful web interface
 * accessible from any device on the same WiFi network.
 * 
 * Hardware Connection:
 * - Pulse sensor signal pin -> A0
 * - Pulse sensor VCC -> 3.3V
 * - Pulse sensor GND -> GND
 * 
 * Features:
 * - Real-time heart rate detection
 * - Beautiful responsive web UI with animations
 * - Serial Monitor output
 * - WiFi connectivity for remote monitoring
 */

// ========================= CONFIGURATION =========================
// WiFi credentials
const char* ssid = "realme 9i";
const char* password = "gragra12345";

// Pulse sensor configuration
const int pulsePin = A0;           // Analog pin for pulse sensor
const int threshold = 512;         // Threshold for beat detection
const int sampleIntervalMs = 20;   // Sample every 20ms (50Hz)
const int beatWindow = 10;         // Number of beats to average

// ========================= GLOBAL VARIABLES =========================
ESP8266WebServer server(80);

// Heart rate calculation variables
int heartRate = 0;
unsigned long lastBeatTime = 0;
unsigned long beatInterval = 0;
int beatsPerMinute = 0;
bool beatDetected = false;
int signalValue = 0;
int peakValue = 0;
int troughValue = 1024;
bool pulseDetected = false;

// Moving average for smoother readings
int beatIntervals[beatWindow];
int beatIndex = 0;
bool beatArrayFilled = false;

// ========================= HEART RATE FUNCTIONS =========================

/**
 * Advanced heart rate detection using peak detection algorithm
 * This function analyzes the pulse sensor signal to detect actual heartbeats
 */
int readHeartRate() {
  static unsigned long lastSampleTime = 0;
  static int lastSignalValue = 0;
  static bool rising = false;
  
  unsigned long currentTime = millis();
  
  // Sample at regular intervals
  if (currentTime - lastSampleTime >= sampleIntervalMs) {
    lastSampleTime = currentTime;
    
    signalValue = analogRead(pulsePin);
    
    // Adaptive threshold based on signal range
    if (signalValue > peakValue) peakValue = signalValue;
    if (signalValue < troughValue) troughValue = signalValue;
    
    // Calculate dynamic threshold
    int dynamicThreshold = troughValue + ((peakValue - troughValue) * 0.6);
    
    // Detect rising edge (beat detection)
    if (signalValue > dynamicThreshold && lastSignalValue <= dynamicThreshold && !rising) {
      rising = true;
      beatDetected = true;
      
      // Calculate time between beats
      if (lastBeatTime > 0) {
        beatInterval = currentTime - lastBeatTime;
        
        // Valid beat interval (30-200 BPM range)
        if (beatInterval > 300 && beatInterval < 2000) {
          // Store in circular buffer for averaging
          beatIntervals[beatIndex] = beatInterval;
          beatIndex = (beatIndex + 1) % beatWindow;
          if (beatIndex == 0) beatArrayFilled = true;
          
          // Calculate average BPM
          int sum = 0;
          int count = beatArrayFilled ? beatWindow : beatIndex;
          for (int i = 0; i < count; i++) {
            sum += beatIntervals[i];
          }
          
          if (count > 0) {
            int avgInterval = sum / count;
            beatsPerMinute = 60000 / avgInterval; // Convert to BPM
            pulseDetected = true;
          }
        }
      }
      lastBeatTime = currentTime;
    }
    
    // Reset rising flag when signal falls
    if (signalValue <= dynamicThreshold) {
      rising = false;
    }
    
    lastSignalValue = signalValue;
    
    // Reset peaks periodically to adapt to changes
    static unsigned long lastResetTime = 0;
    if (currentTime - lastResetTime > 5000) { // Reset every 5 seconds
      peakValue = max(signalValue, 512);
      troughValue = min(signalValue, 512);
      lastResetTime = currentTime;
    }
  }
  
  // Return current BPM or 0 if no valid reading
  return pulseDetected ? beatsPerMinute : 0;
}

// ========================= WEB UI FUNCTIONS =========================

/**
 * Generate beautiful HTML UI with real-time heart rate display
 * Features: Animated heart, live BPM updates, pulse waveform, status indicators
 */
String getHTML() {
  String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>❤️ Heart Rate Monitor</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333; 
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .container { 
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px; 
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            padding: 40px;
            max-width: 450px;
            width: 90%;
            text-align: center;
            backdrop-filter: blur(10px);
        }
        
        .header { margin-bottom: 30px; }
        .title { font-size: 28px; font-weight: 700; color: #2c3e50; margin-bottom: 10px; }
        .subtitle { color: #7f8c8d; font-size: 16px; }
        
        .heart-section { margin: 30px 0; }
        .heart { 
            font-size: 80px; 
            animation: heartbeat 1.2s ease-in-out infinite;
            display: inline-block;
            filter: drop-shadow(0 4px 8px rgba(231, 76, 60, 0.3));
        }
        
        @keyframes heartbeat { 
            0%, 100% { transform: scale(1); }
            14% { transform: scale(1.1); }
            28% { transform: scale(1); }
            42% { transform: scale(1.1); }
            70% { transform: scale(1); }
        }
        
        .bpm-section { margin: 30px 0; }
        .bpm-value { 
            font-size: 56px; 
            font-weight: 800; 
            color: #e74c3c; 
            margin: 10px 0;
            text-shadow: 0 2px 4px rgba(231, 76, 60, 0.2);
        }
        .bpm-label { 
            font-size: 18px; 
            color: #7f8c8d; 
            font-weight: 600;
            letter-spacing: 2px;
        }
        
        .status { 
            display: inline-block;
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            font-weight: 600;
            margin: 15px 0;
        }
        .status.detecting { background: #3498db; color: white; }
        .status.connected { background: #27ae60; color: white; }
        .status.error { background: #e74c3c; color: white; }
        
        .pulse-wave { 
            margin: 20px 0;
            height: 60px;
            background: #ecf0f1;
            border-radius: 10px;
            position: relative;
            overflow: hidden;
        }
        
        .wave-line {
            position: absolute;
            top: 50%;
            left: 0;
            right: 0;
            height: 2px;
            background: #e74c3c;
            transform: translateY(-50%);
        }
        
        .pulse-dot {
            position: absolute;
            width: 8px;
            height: 8px;
            background: #e74c3c;
            border-radius: 50%;
            top: 50%;
            transform: translateY(-50%);
            animation: pulse-move 2s linear infinite;
        }
        
        @keyframes pulse-move {
            0% { left: -10px; }
            100% { left: 100%; }
        }
        
        .stats { 
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-top: 30px;
        }
        
        .stat-item {
            background: #f8f9fa;
            padding: 15px;
            border-radius: 10px;
            border-left: 4px solid #e74c3c;
        }
        
        .stat-value { font-size: 24px; font-weight: 700; color: #2c3e50; }
        .stat-label { font-size: 12px; color: #7f8c8d; text-transform: uppercase; }
        
        .footer { 
            margin-top: 30px; 
            color: #bdc3c7; 
            font-size: 14px; 
        }
        
        .connection-info {
            background: #f1f2f6;
            padding: 10px;
            border-radius: 8px;
            margin-top: 20px;
            font-size: 12px;
            color: #57606f;
        }
        
        @media (max-width: 480px) {
            .container { padding: 20px; }
            .heart { font-size: 60px; }
            .bpm-value { font-size: 48px; }
            .stats { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="title">❤️ Heart Rate Monitor</div>
            <div class="subtitle">Real-time pulse monitoring</div>
        </div>
        
        <div class="heart-section">
            <div class="heart" id="heartIcon">❤️</div>
        </div>
        
        <div class="bpm-section">
            <div class="bpm-value" id="bpmValue">--</div>
            <div class="bpm-label">BPM</div>
        </div>
        
        <div class="status detecting" id="status">Detecting pulse...</div>
        
        <div class="pulse-wave">
            <div class="wave-line"></div>
            <div class="pulse-dot" id="pulseDot"></div>
        </div>
        
        <div class="stats">
            <div class="stat-item">
                <div class="stat-value" id="signalStrength">--</div>
                <div class="stat-label">Signal</div>
            </div>
            <div class="stat-item">
                <div class="stat-value" id="lastUpdate">--</div>
                <div class="stat-label">Updated</div>
            </div>
        </div>
        
        <div class="connection-info">
            <div>📡 Connected to: <strong>)" + String(ssid) + R"(</strong></div>
            <div>🌐 Device IP: <strong id="deviceIP">)" + WiFi.localIP().toString() + R"(</strong></div>
        </div>
        
        <div class="footer">
            <p>ESP8266 Heart Rate Monitor</p>
        </div>
    </div>

    <script>
        let lastBPM = 0;
        let isConnected = true;
        
        function updateTime() {
            const now = new Date();
            const timeStr = now.toLocaleTimeString();
            document.getElementById('lastUpdate').textContent = timeStr.split(' ')[0];
        }
        
        function fetchData() {
            const startTime = Date.now();
            
            Promise.all([
                fetch('/bpm').then(r => r.text()),
                fetch('/signal').then(r => r.text()),
                fetch('/status').then(r => r.text())
            ])
            .then(([bpm, signal, status]) => {
                const bpmValue = parseInt(bpm);
                const signalValue = parseInt(signal);
                const statusElement = document.getElementById('status');
                // Update BPM display
                document.getElementById('bpmValue').textContent = bpmValue > 0 ? bpmValue : '--';
                document.getElementById('signalStrength').textContent = signalValue;
                // Show 'Detecting pulse...' only when beat detected, else 'No beat found'
                if (status === 'connected') {
                    statusElement.textContent = 'Detecting pulse...';
                    statusElement.className = 'status connected';
                } else {
                    statusElement.textContent = 'No beat found';
                    statusElement.className = 'status error';
                }
                // Animate heart on beat change
                if (bpmValue !== lastBPM && bpmValue > 0) {
                    const heart = document.getElementById('heartIcon');
                    heart.style.animation = 'none';
                    setTimeout(() => {
                        heart.style.animation = 'heartbeat 1.2s ease-in-out infinite';
                    }, 10);
                }
                lastBPM = bpmValue;
                isConnected = true;
                updateTime();
            })
            .catch(error => {
                console.error('Connection error:', error);
                isConnected = false;
                document.getElementById('status').textContent = 'Connection lost ⚠️';
                document.getElementById('status').className = 'status error';
            });
        }
        
        // Update every second
        setInterval(fetchData, 1000);
        setInterval(updateTime, 1000);
        
        // Initial load
        fetchData();
        updateTime();
    </script>
</body>
</html>
  )";
  return html;
}

// ========================= SERVER HANDLERS =========================

/**
 * Handle root URL - serve the main web interface
 */
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

/**
 * Handle /bpm endpoint - return current heart rate
 */
void handleBPM() {
  server.send(200, "text/plain", String(heartRate));
}

/**
 * Handle /signal endpoint - return raw signal strength
 */
void handleSignal() {
  server.send(200, "text/plain", String(signalValue));
}

/**
 * Handle /status endpoint - return sensor status
 */
void handleStatus() {
  String status = pulseDetected ? "connected" : "detecting";
  server.send(200, "text/plain", status);
}

/**
 * Handle /data endpoint - return JSON with all data
 */
void handleData() {
  String json = "{";
  json += "\"bpm\":" + String(heartRate) + ",";
  json += "\"signal\":" + String(signalValue) + ",";
  json += "\"detected\":" + String(pulseDetected ? "true" : "false") + ",";
  json += "\"timestamp\":" + String(millis());
  json += "}";
  server.send(200, "application/json", json);
}

// ========================= MAIN PROGRAM =========================

void setup() {
  Serial.begin(115200);
  delay(10);
  
  Serial.println("\n=== ESP8266 Heart Rate Monitor ===");
  Serial.println("Initializing...");
  
  // Initialize pulse sensor pin
  pinMode(pulsePin, INPUT);
  
  // Initialize beat interval array
  for (int i = 0; i < beatWindow; i++) {
    beatIntervals[i] = 0;
  }
  
  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
  } else {
    Serial.println("\n✗ WiFi connection failed!");
    Serial.println("Please check your credentials and try again.");
  }
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/bpm", handleBPM);
  server.on("/signal", handleSignal);
  server.on("/status", handleStatus);
  server.on("/data", handleData);
  
  // Start web server
  server.begin();
  Serial.println("✓ HTTP server started on port 80");
  Serial.println("\nAccess the heart rate monitor at:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("\n=== Monitoring Started ===");
  Serial.println("BPM | Signal | Status");
  Serial.println("----+--------+--------");
}

void loop() {
  // Read heart rate from sensor
  heartRate = readHeartRate();
  
  // Handle web server requests
  server.handleClient();
  
  // Print to Serial Monitor (every second to avoid spam)
  static unsigned long lastSerialPrint = 0;
  if (millis() - lastSerialPrint >= 1000) {
    lastSerialPrint = millis();
    
    // Format output for better readability
    char buffer[50];
    sprintf(buffer, "%3d | %4d   | %s", 
            heartRate > 0 ? heartRate : 0, 
            signalValue,
            pulseDetected ? "DETECTED" : "SEARCHING");
    Serial.println(buffer);
    
    // Show beat detection indicator
    if (beatDetected) {
      Serial.println("    ❤️ BEAT!");
      beatDetected = false; // Reset flag
    }
  }
  
  // Small delay to prevent overwhelming the system
  delay(10);
}