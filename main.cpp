#include <WiFi.h>        // Include the WiFi library for network connectivity
#include <time.h>        // Include the time library for date and time functionalities
#include <WebServer.h>   // Include the WebServer library to create a web server
#include <ESPmDNS.h>     // Include mDNS library for DNS services
#include <ArduinoJson.h> // Include the ArduinoJson library for handling JSON data
#include "ACS712.h"      // Include the ACS712 library for current sensing

// WiFi credentials and mDNS hostname
const char *ssid = "ESP32-AP";     // SSID for the WiFi network
const char *password = "12345678"; // Password for the WiFi network
const char *hostname = "iotbulb";  // mDNS hostname for the device (URL)

// Static IP setup
IPAddress localIP(192, 168, 4, 1);  // Default local IP address for the ESP32 in AP mode
IPAddress gateway(192, 168, 4, 1);   // Default gateway, same as local IP in AP mode
IPAddress subnet(255, 255, 255, 0);  // Subnet mask for the network

// Web server on port 80
WebServer server(80); // Create a web server instance listening on port 80

// Pin Definitions
const int relay1Pin = 33;        // Pin for Relay 1, controlling Bulb 1
const int relay2Pin = 25;        // Pin for Relay 2, controlling Bulb 2
const int currentSensorPin = 35; // Pin for the current sensor to monitor current flow
const int greenLEDPin = 21;      // Pin for the green LED indicating system readiness
const int yellowLEDPin = 19;     // Pin for the yellow LED indicating idle state
const int redLEDPin = 18;        // Pin for the red LED indicating an error state

// States
bool bulb1State = false;          // Variable to store the state of Bulb 1 (On/Off)
bool bulb2State = false;          // Variable to store the state of Bulb 2 (On/Off)
bool isAnyBulbOn = false;         // Flag to check if any bulb is currently on
bool isScheduled = false;         // Flag to check if a schedule is set for operations
int scheduledTime = 5;            // Default scheduled time in seconds for bulb operations
unsigned long lastUpdateTime = 0; // Variable to track the last update time for scheduled tasks
unsigned long relayOnTime = 0;    // Timer to track how long the relay has been activated

// Historical data structure definition
struct HistoricalData {
    String date;                // Date of the data entry
    String time;                // Time of the data entry
    String bulb1State;          // State of Bulb 1 (On/Off)
    String bulb2State;          // State of Bulb 2 (On/Off)
    float roundedCurrent;       // Current reading from the current sensor, rounded
    float roundedPower;         // Calculated power consumption based on current readings
};

const int maxHistoryRows = 10;          // Maximum number of rows for historical data
HistoricalData history[maxHistoryRows]; // Array to store historical data entries
int historyIndex = 0;                    // Current index for inserting new historical data entries

// Time Related Variables
String storedDate = "2024-11-1"; // Default date in format YYYY-MM-DD
String storedTime = "00:00:00";  // Default time in format HH:MM:SS
bool timeInitialized = false;    // Flag to check if the time has been initialized

// Define variables for the ACS712 5A current sensor
ACS712 current_Sensor(ACS712_05B, currentSensorPin); // Create an instance of the current sensor
const float voltageReference = 3.3;                  // Reference voltage for the ESP32 in volts
const float voltageSupply = 220.0;                   // Supply voltage (AC) in volts

// Current and Power Variable Holders
float currentReading;   // Variable to store the current reading in Amperes
float powerConsumption; // Variable to store power consumption in Watts

// Function prototypes with brief descriptions
void updateDateTime();                           // Update the date and time from the network
void handleRoot();                               // Handle requests to the root URL by serving HTML, CSS, and JS
void handleTurnOnAll();                          // Handle request to turn on all bulbs simultaneously
void handleTurnOffAll();                         // Handle request to turn off all bulbs simultaneously
void handleToggleBulb1();                        // Handle request to toggle the state of Bulb 1
void handleToggleBulb2();                        // Handle request to toggle the state of Bulb 2
void handleScheduleTime();                       // Handle request to set a scheduled time for operations
void handleTimeInit();                           // Handle request to initialize the date and time
void handleHistoricalData();                     // Serve historical data as JSON
void updateHistoricalData();                     // Update the historical data array with new entries
void setLEDs(bool ready, bool idle, bool error); // Control LED indicators based on system state

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Light Bulb Control</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: #121212;
            color: #fff;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: auto;
            padding: 20px;
            background: #1e1e1e;
            border-radius: 8px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.3);
        }
        h1 {
            text-align: center;
            font-size: 2.5em;
            margin-bottom: 20px;
        }
        .main-body {
            display: flex;
            flex-direction: row;
        }
        .left-column {
            width: 25%;
            padding: 10px;
        }
        .right-column {
            width: 75%;
            padding: 10px;
        }
        .bulb-control, .scheduling {
            margin: 20px 0;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        .circular-button {
            margin-top: 10px;
            padding: 15px;
            font-size: 20px;
            cursor: pointer;
            background: #4caf50;
            border: none;
            border-radius: 50%;
            width: 60px;
            height: 60px;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: background-color .3s ease, transform .2s ease;
        }
        .circular-button:hover {
            background: #45a049;
        }
        input[type="range"] {
            margin-top: 10px;
        }
        .submit-button {
            margin-top: 10px;
            background: #4caf50;
            border: none;
            border-radius: 10px; /* Rounded edges */
            width: 100px; /* Adjust width */
            height: 40px; /* Adjust height */
            cursor: pointer;
            transition: background-color .3s ease;
            font-size: 22px;
        }
        .submit-button:hover {
            background: #45a049;
        }
        .slider-label {
            text-align: center; /* Center align label */
        }
        .slider-min-max {
            display: flex;
            justify-content: space-between; /* Space out min and max labels */
            width: 100%; /* Full width under the slider */
        }
        table {
            width: 100%;
            max-width: 100%; /* Ensure table does not exceed container width */
            border-collapse: collapse;
            margin-top: 10px;
        }
        th, td {
            padding: 10px;
            border: 1px solid #333;
            text-align: center;
            font-size: 14px; /* Default font size for desktop */
        }
        th {
            background: #333;
        }
        tbody tr:nth-child(even) {
            background: #1e1e1e;
        }

        /* Media queries for scaling */
        @media (max-width: 800px) {
            .container {
                padding: 15px;
            }
            h1 {
                font-size: 2em;
            }
            th, td {
                font-size: 12px; /* Slightly smaller for tablet/large mobile */
            }
            table {
                margin: 0; /* Remove margin on smaller screens */
            }
        }
        @media (max-width: 600px) {
            .container {
                padding: 4px;
            }
            h1 {
                font-size: 2.0em;
            }
            .main-body {
                flex-direction: column; /* Stack columns on smaller screens */
            }
            .left-column, .right-column {
                width: 100%; /* Full width for both columns */
                padding: 5px;
            }
            .circular-button {
                width: 50px;
                height: 50px;
                font-size: 16px;
            }
            th, td {
                padding: 5px;
                font-size: 10px; 
            }
            table {
                margin: 0; /* Remove margin on smaller screens */
            }
        }
        @media (max-width: 400px) {
            .container {
                padding: 0px;
            }
            h1 {
                font-size: 1.8em;
            }
            .circular-button {
                width: 45px;
                height: 45px;
                font-size: 12px;
            }
            th, td {
                padding: 2px;
                font-size: 8px; 
            }
            table {
                margin: 0; /* Remove margin on smaller screens */
            }
        }
    </style>
</head>

<body>
    <div class="container">
        <h1>IoT Light Bulb Project</h1>
        <div class="main-body">
            <div class="left-column">
                <h2 style="text-align:center;">Controls</h2>
                <!-- Bulb control buttons -->
                <div class="bulb-control">
                    <label>Turn On All Bulbs</label>
                    <button id="turnOnAll" class="circular-button">🔆</button>
                </div>
                <div class="bulb-control">
                    <label>Toggle Bulb 1</label>
                    <button id="toggleBulb1" class="circular-button">🔄</button>
                </div>
                <div class="bulb-control">
                    <label>Toggle Bulb 2</label>
                    <button id="toggleBulb2" class="circular-button">🔄</button>
                </div>
                <div class="bulb-control">
                    <label>Turn Off All Bulbs</label>
                    <button id="turnOffAll" class="circular-button">🔅</button>
                </div>

                <!-- Schedule timing -->
                <div class="scheduling">
                    <label class="slider-label">Schedule Bulb Time (seconds):</label>
                    <input type="range" id="scheduleSlider" min="1" max="60" value="5" oninput="updateScheduleValue()">
                    <div class="slider-min-max">
                        <span>1</span>
                        <span>60</span>
                    </div>
                    <div id="scheduleDisplay" class="current-value">5 seconds</div>
                    <button id="submitSchedule" class="submit-button">✔️</button>
                </div>
            </div>
            <div class="right-column">
                <h2 style="text-align:center;">Historical Data</h2>
                <table>
                    <thead>
                        <tr>
                            <th>Date</th>
                            <th>Time</th>
                            <th>Bulb 1 State</th>
                            <th>Bulb 2 State</th>
                            <th>Current<br>(A)</th>
                            <th>Power<br>(W)</th>
                        </tr>
                    </thead>
                    <tbody id="dataRows"></tbody>
                </table>
            </div>
        </div>
    </div>
    <script>
        const esp32Ip = '192.168.4.1'; // Replace with your ESP32's IP address
        let updateInterval; // To hold the interval ID

        // Updates schedule display and sends the schedule command
        function updateScheduleValue() {
            const scheduleValue = document.getElementById("scheduleSlider").value;
            document.getElementById("scheduleDisplay").innerText = scheduleValue + " seconds";
        }

        // Attach event listeners to buttons
        document.getElementById("turnOnAll").onclick = () => sendCommand('turnOnAll');
        document.getElementById("toggleBulb1").onclick = () => sendCommand('toggleBulb1');
        document.getElementById("toggleBulb2").onclick = () => sendCommand('toggleBulb2');
        document.getElementById("turnOffAll").onclick = () => sendCommand('turnOffAll');


        // Attach event listeners for submit buttons for schedule slider
        document.getElementById("submitSchedule").onclick = () => {
            const scheduleValue = document.getElementById("scheduleSlider").value;
            sendCommand('schedule', scheduleValue); // Send schedule value
        };

        // Send commands to the ESP32 with parameters
        function sendCommand(command, value = null) {
            const url = value !== null ? `http://${esp32Ip}/${command}?value=${value}` : `http://${esp32Ip}/${command}`;

            fetch(url)
                .then(response => {
                    if (!response.ok) {
                        throw new Error(`Network response was not ok: ${response.statusText}`);
                    }
                    return response.json(); // Parse JSON response
                })
                .then(data => {
                    console.log("Response from ESP32:", data);
                })
                .catch(error => console.error("Fetch error:", error));
        }
        
        // Function to fetch and update historical data
        function fetchHistoricalData() {
            fetch(`http://${esp32Ip}/historicalData`) // Ensure this matches your endpoint
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.json();
                })
                .then(data => {
                    const dataRows = document.getElementById("dataRows");
                    dataRows.innerHTML = ""; // Clear existing rows

                    // Populate the table with new data
                    if (Array.isArray(data.data) && data.data.length > 0) {
                        data.data.forEach(entry => {
                            const row = document.createElement("tr");
                            row.innerHTML = `
                                <td>${entry.date}</td>
                                <td>${entry.time}</td>
                                <td>${entry.bulb1State}</td>
                                <td>${entry.bulb2State}</td>
                                <td>${entry.current}</td>
                                <td>${entry.power}</td>
                            `;
                            dataRows.appendChild(row);
                        });
                    } else {
                        const row = document.createElement("tr");
                        row.innerHTML = `<td colspan="6">No historical data available</td>`;
                        dataRows.appendChild(row);
                    }
                })
                .catch(error => console.error("Fetch error:", error));
        }

        // Set an interval to fetch historical data every second
        setInterval(fetchHistoricalData, 5000); // Fetch latest data every 5 second    

        // Call this function when a device connects (like in an event listener)
        function initializeTime() {
            const now = new Date();
            const formattedDate = now.toISOString().split('T')[0]; // YYYY-MM-DD
            const formattedTime = now.toTimeString().split(' ')[0]; // HH:MM:SS
            
            fetch(`http://${esp32Ip}/timeInit?date=${formattedDate}&time=${formattedTime}`)
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.text();
                })
                .then(data => console.log(data))
                .catch(error => console.error("Fetch error:", error));
        }

        // Call this function on load or when the first device connects
        initializeTime();
    </script>
</body>

</html>
)rawliteral";

// Setup or Configure initial parameters
void setup() {
    // Initialize Serial Communication
    Serial.begin(115200);

    // Set relay and LED pins as outputs to control the bulbs and indicators
    pinMode(relay1Pin, OUTPUT);        // Relay for bulb 1
    pinMode(relay2Pin, OUTPUT);        // Relay for bulb 2
    pinMode(greenLEDPin, OUTPUT);      // Green LED pin
    pinMode(yellowLEDPin, OUTPUT);     // Yellow LED pin
    pinMode(redLEDPin, OUTPUT);        // Red LED pin
    pinMode(currentSensorPin, INPUT);  // Current sensor input pin

    // Start the ESP32 as an access point with the specified SSID and password
    WiFi.softAP(ssid, password);
    // Configure the access point with a static IP, gateway, and subnet
    WiFi.softAPConfig(localIP, gateway, subnet);
    Serial.println("ESP32 Access Point started");

    // Start mDNS service to allow easy access using a hostname
    MDNS.begin(hostname);
    Serial.println("mDNS service started");

    // Set up the time using NTP (Network Time Protocol)
    configTime(28800, 0, "pool.ntp.org", "time.nist.gov"); // 28800 seconds = UTC+8

    // Define server routes for handling different requests
    server.on("/", handleRoot);  // Root URL request
    server.on("/turnOnAll", handleTurnOnAll);  // Request to turn on all bulbs
    server.on("/turnOffAll", handleTurnOffAll);  // Request to turn off all bulbs
    server.on("/toggleBulb1", handleToggleBulb1);  // Request to toggle bulb 1
    server.on("/toggleBulb2", handleToggleBulb2);  // Request to toggle bulb 2
    server.on("/schedule", handleScheduleTime);  // Request to set a schedule
    server.on("/timeInit", handleTimeInit);  // Request to initialize time
    server.on("/historicalData", handleHistoricalData);  // Request to get historical data

    // Start the server to listen for incoming requests
    server.begin();
    Serial.println("Server started");

    // Set initial state of the LEDs (Idle state)
    setLEDs(false, true, false);  // Green off, Yellow on, Red off

    // Calibrate the current sensor if necessary
    current_Sensor.calibrate();
}

// Main loop
void loop() {
    // Handle incoming client requests
    server.handleClient();

    // Check if a scheduled time for toggling bulbs is set
    if (isScheduled) {
        // Check if the scheduled time has passed
        if (millis() - relayOnTime >= (scheduledTime * 1000)) {
            handleTurnOffAll();  // Turn off all bulbs if scheduled time is reached
        }
    }

    // Check if it's time to send updated historical data every 5 seconds
    unsigned long currentMillis = millis();  // Get the current time in milliseconds
    if (currentMillis - lastUpdateTime >= 5000) {
        updateHistoricalData();         // Update historical data
        lastUpdateTime = currentMillis; // Update the last update time
        handleHistoricalData();         // Call the function to send historical data
    }
}

// Function to handle root URL requests
void handleRoot() {
    Serial.println("Handling root request");  // Log the request handling
    server.send(200, "text/html", MAIN_page); // Send the main HTML page as response
}

// Function to turn on all bulbs
void handleTurnOnAll() {
    Serial.println("Turning on all bulbs");                                       // Log the action
    bulb1State = true;                                                            // Set bulb 1 state to on
    bulb2State = true;                                                            // Set bulb 2 state to on
    digitalWrite(relay1Pin, HIGH);                                                // Activate relay for bulb 1
    digitalWrite(relay2Pin, HIGH);                                                // Activate relay for bulb 2
    setLEDs(true, false, false);                                                  // Set LEDs: Green on, Yellow off, Red off
    server.send(200, "application/json", "{\"status\":\"All bulbs turned on\"}"); // Send success response
}

// Function to turn off all bulbs
void handleTurnOffAll() {
    Serial.println("Turning off all bulbs");                                       // Log the action
    bulb1State = false;                                                            // Set bulb 1 state to off
    bulb2State = false;                                                            // Set bulb 2 state to off
    digitalWrite(relay1Pin, LOW);                                                  // Deactivate relay for bulb 1
    digitalWrite(relay2Pin, LOW);                                                  // Deactivate relay for bulb 2
    setLEDs(false, true, false);                                                   // Set LEDs: Green off, Yellow on, Red off
    isScheduled = false;                                                           // Clear any scheduled actions
    server.send(200, "application/json", "{\"status\":\"All bulbs turned off\"}"); // Send success response
}

// Function to toggle Bulb 1
void handleToggleBulb1() {
    Serial.println("Toggling Bulb 1");                                       // Log the action
    bulb1State = !bulb1State;                                                // Toggle the state of bulb 1
    digitalWrite(relay1Pin, bulb1State ? HIGH : LOW);                        // Set relay based on the new state
    
    // Determine LED states based on bulb states
    bool isAnyBulbOn = bulb1State || bulb2State;                             // True if any bulb is on
    bool areBothBulbsOff = !bulb1State && !bulb2State;                       // True if both bulbs are off
    
    setLEDs(isAnyBulbOn, areBothBulbsOff, false);                            // Set LED states accordingly
    server.send(200, "application/json", "{\"status\":\"Bulb 1 toggled\"}"); // Send success response
}

// Function to toggle Bulb 2
void handleToggleBulb2() {
    Serial.println("Toggling Bulb 2");                                       // Log the action
    bulb2State = !bulb2State;                                                // Toggle the state of bulb 2
    digitalWrite(relay2Pin, bulb2State ? HIGH : LOW);                        // Set relay based on the new state

    // Determine LED states based on bulb states
    bool isAnyBulbOn = bulb1State || bulb2State;                             // True if any bulb is on
    bool areBothBulbsOff = !bulb1State && !bulb2State;                       // True if both bulbs are off

    setLEDs(isAnyBulbOn, areBothBulbsOff, false);                            // Set LED states accordingly
    server.send(200, "application/json", "{\"status\":\"Bulb 2 toggled\"}"); // Send success response
}

// Function to schedule bulb time and turn off if scheduled time has passed
void handleScheduleTime() {
    Serial.println("Received schedule request"); // Log the received schedule request
    handleTurnOnAll();                           // Turn on the bulbs immediately when scheduling

    // Check if a "value" parameter was provided in the request
    if (server.hasArg("value")) {
        scheduledTime = server.arg("value").toInt(); // Convert the parameter to an integer for scheduled time
        relayOnTime = millis();                      // Record the current time to track when the bulbs were turned on
        isScheduled = true;                          // Mark that a schedule has been set

        // Send a success response back to the client
        server.send(200, "application/json", "{\"status\":\"success\"}");
        Serial.println("Scheduled time set to: " + String(scheduledTime) + " seconds."); // Log the scheduled time
    } else {
        // If the parameter is missing, send an error response
        server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Missing parameter\"}");
        Serial.println("Error: Missing time parameter"); // Log the error
    }
}

// Function to initialize the date and time for display
void handleTimeInit() {
    // Check if the time has not been initialized and both date and time are provided
    if (!timeInitialized && server.hasArg("date") && server.hasArg("time")) { 
        storedDate = server.arg("date");                                      // Get date from the request
        storedTime = server.arg("time");                                      // Get time from the request
        timeInitialized = true;                                               // Set the flag to indicate time is initialized
        Serial.println("Time initialized: " + storedDate + " " + storedTime); // Log the initialized time
    }
    server.send(200, "text/plain", "Time initialized");                       // Respond to the client indicating success
}

// Function to format current readings to 4 decimal places
String formatCurrent(float value) {
    return String(floor(value * 10000) / 10000.0, 4); // Formats the value to 4 decimal places
}

// Function to format power readings to 2 decimal places
String formatPower(float value) {
    return String(floor(value * 100) / 100.0, 2); // Formats the value to 2 decimal places
}

// Function to handle historical data retrieval
void handleHistoricalData() {
    DynamicJsonDocument jsonDoc(2048);                       // Create a JSON document with a capacity of 2048 bytes
    JsonArray dataArray = jsonDoc.createNestedArray("data"); // Create a nested array for JSON response

    // Add historical entries to the JSON array, preserving the order
    for (int i = 0; i < maxHistoryRows; i++) {
        // Calculate index to fetch the latest data first using modulo for wrapping around
        int index = (historyIndex - 1 - i + maxHistoryRows) % maxHistoryRows; 
        JsonObject entry = dataArray.createNestedObject();               // Create a new JSON object for each entry
        entry["date"] = history[index].date;                             // Add date entry
        entry["time"] = history[index].time;                             // Add time entry
        entry["bulb1State"] = history[index].bulb1State;                 // Add state of Bulb 1
        entry["bulb2State"] = history[index].bulb2State;                 // Add state of Bulb 2
        entry["current"] = formatCurrent(history[index].roundedCurrent); // Format and add current value
        entry["power"] = formatPower(history[index].roundedPower);       // Format and add power value
    }
    
    String response;                                // Declare a string to hold the serialized JSON response
    serializeJson(jsonDoc, response);               // Serialize the JSON document into the response string
    server.send(200, "application/json", response); // Send the JSON response back to the client
}

// Function to update historical data array with new entries
void updateHistoricalData() {
    String date = storedDate; // Use stored date for the new entry
    String time = storedTime; // Use stored time for the new entry

    // Read the current from the ACS712 current sensor
    currentReading = current_Sensor.getCurrentAC(); // Get the current in AC

    // Ignore very small currents (below a threshold to avoid noise)
    if (currentReading < 0.09) { // Adjust this threshold based on your needs
        currentReading = 0.0;    // Set to zero if below the threshold
    }

    // Calculate power consumption in watts
    powerConsumption = voltageSupply * currentReading; // Power in Watts

    // Enforce strict decimal precision for current and power
    float roundedCurrent = floor(currentReading * 10000) / 10000.0; // Keep only 4 decimal places
    float roundedPower = floor(powerConsumption * 100) / 100.0;     // Keep only 2 decimal places

    // Log formatted current and power values
    Serial.print("Current (A): "); Serial.println(formatCurrent(currentReading)); // Log current
    Serial.print(", Power (W): "); Serial.println(formatPower(powerConsumption)); // Log power

    // Update the historical data entry with the latest values
    history[historyIndex] = {date, time, bulb1State ? "On" : "Off", bulb2State ? "On" : "Off", roundedCurrent, roundedPower}; 
    historyIndex = (historyIndex + 1) % maxHistoryRows; // Update index and wrap around if at max size
}

// Function to control the LED indicators
void setLEDs(bool ready, bool idle, bool error) {
    // Set LED states based on the parameters
    digitalWrite(greenLEDPin, ready ? HIGH : LOW); // Set green LED for ready state
    digitalWrite(yellowLEDPin, idle ? HIGH : LOW); // Set yellow LED for idle state
    digitalWrite(redLEDPin, error ? HIGH : LOW);   // Set red LED for error state
}