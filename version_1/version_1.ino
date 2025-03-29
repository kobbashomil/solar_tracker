#include <WiFi.h>
#include <WebServer.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>

// ----------------------- Configuration -----------------------
const char* ssid = "solar_track";
const char* password = "admin70503";

const int RELAY_EAST = 12;
const int RELAY_WEST = 14;
const int SENSOR_EAST = 22;
const int SENSOR_WEST = 23;


bool autoMode = true;
int morningStartHour = 6;
int nightReturnHour = 18;
int stepInterval = 60;
int motorStepTime = 5000;  // متغير يمكن تحديثه أثناء التشغيل

unsigned long lastMoveTime = 0;  // Stores last movement time
bool returningToEast = false;    // Track if we returned to East


ThreeWire myWire(15, 2, 4);  
RtcDS1302<ThreeWire> Rtc(myWire);
WebServer server(80);

// ----------------------- WiFi Access Point Setup -----------------------
void setupWiFi() {
    WiFi.softAP(ssid, password);
    Serial.print("Access Point IP Address: ");
    Serial.println(WiFi.softAPIP());
}

// ----------------------- Motor Control Functions -----------------------
void moveEast() {
    if (digitalRead(SENSOR_EAST) == HIGH) {
        Serial.println("East limit reached");
        return;
    }
    Serial.println("Moving East");
    unsigned long startTime = millis();
    digitalWrite(RELAY_EAST, HIGH);
    while (millis() - startTime < motorStepTime) {
        server.handleClient(); // يسمح بمعالجة الطلبات أثناء تشغيل المحرك
    }
    digitalWrite(RELAY_EAST, LOW);
}

void moveWest() {
    if (digitalRead(SENSOR_WEST) == HIGH) {
        Serial.println("West limit reached");
        return;
    }
    Serial.println("Moving West");
    unsigned long startTime = millis();
    digitalWrite(RELAY_WEST, HIGH);
    while (millis() - startTime < motorStepTime) {
        server.handleClient(); // يسمح بمعالجة الطلبات أثناء تشغيل المحرك
    }
    digitalWrite(RELAY_WEST, LOW);
}



void stopMotor() {
    Serial.println("Stopping Motor");
    digitalWrite(RELAY_EAST, LOW);
    digitalWrite(RELAY_WEST, LOW);
}

// ----------------------- Web Server Handlers -----------------------

void handleRoot() {
    RtcDateTime now = Rtc.GetDateTime();
    char timeBuffer[20];
    sprintf(timeBuffer, "%02d:%02d:%02d", now.Hour(), now.Minute(), now.Second());

    String html = "<!DOCTYPE html><html><head><title>Solar Controller</title>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<style>"
                  "body {text-align:center; font-family:Arial, sans-serif; background:#f4f4f4; color:#333; padding:20px;}"
                  "h1 {color:#007bff; margin-bottom: 10px;}"
                  ".container {max-width: 600px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1);}"
                  ".btn {display: inline-block; padding: 15px 30px; margin: 10px; border: none; cursor: pointer; color: white; border-radius: 5px; font-size: 16px;}"
                  ".btn.east {background: #28a745;}"
                  ".btn.west {background: #dc3545;}"
                  ".btn.save {background: #007bff; /* اللون الأزرق */color: white; /* لون النص */}"
                  ".btn.time {background: #007bff; /* اللون الأزرق */color: white; /* لون النص */}"
                  ".settings-box {padding: 15px; background: #f8f9fa; border-radius: 10px; margin-top: 20px; text-align: left;}"
                  "form {margin-top: 20px;}"
                  "label {display: block; margin: 8px 0; font-weight: bold;}"
                  "input {width: 100%; padding: 8px; margin: 5px 0; border: 1px solid #ccc; border-radius: 5px;}"
                  "input[type='submit'] {width: auto; padding: 10px 15px; cursor: pointer;}"
                  "</style>"

                  "<script>"
                  "function move(dir, action) {"
                  " var xhttp = new XMLHttpRequest();"
                  " xhttp.open('GET', '/move?dir=' + dir + '&action=' + action, true);"
                  " xhttp.send();"
                  "}"
                  
                  "document.addEventListener('DOMContentLoaded', function() {"
                  " function addControlEvents(buttonId, direction) {"
                  " var button = document.getElementById(buttonId);"
                  " if (button) {"
                  " button.addEventListener('mousedown', function(event) { event.preventDefault(); move(direction, 'start'); });"
                  " button.addEventListener('mouseup', function(event) { event.preventDefault(); move(direction, 'stop'); });"
                  " button.addEventListener('touchstart', function(event) { event.preventDefault(); move(direction, 'start'); });"
                  " button.addEventListener('touchend', function(event) { event.preventDefault(); move(direction, 'stop'); });"
                  " }"
                  " }"
                  
                  " addControlEvents('btn-east', 'east');"
                  " addControlEvents('btn-west', 'west');"
                  "});"
                  "</script>"

                  "</head><body>"
                  "<div class='container'>"
                  "<h1>Solar Panel Controller</h1>"
                  "<p><strong>Current Time:</strong> " + String(timeBuffer) + "</p>"
                  
                  "<button id='btn-east' class='btn east'>Move East</button>"
                  "<button id='btn-west' class='btn west'>Move West</button>"

                  "<div class='settings-box'>"
                  "<h2>Settings</h2>"
                  "<form action='/settings' method='POST'>"
                  "<label>Auto Mode: <input type='checkbox' name='autoMode' " + (autoMode ? "checked" : "") + "></label>"
                  "<label>Morning Start Hour: <input type='number' name='morningStart' value='" + String(morningStartHour) + "'></label>"
                  "<label>Night Return Hour: <input type='number' name='nightReturn' value='" + String(nightReturnHour) + "'></label>"
                  "<label>Step Interval (min): <input type='number' name='stepInterval' value='" + String(stepInterval) + "'></label>"
                  "<label>Motor Step Time (ms): <input type='number' name='motorStepTime' value='" + String(motorStepTime) + "'></label>"
                  "<input type='submit' value='Save Settings' class='btn save'></form>"
                  "</div>"

                  "<div class='settings-box'>"
                  "<h2>Set Time</h2>"
                  "<form action='/settime' method='POST'>"
                  "<label>Hour: <input type='number' name='hour' min='0' max='23'></label>"
                  "<label>Minute: <input type='number' name='minute' min='0' max='59'></label>"
                  "<label>Second: <input type='number' name='second' min='0' max='59'></label>"
                  "<label>Day: <input type='number' name='day' min='1' max='31'></label>"
                  "<label>Month: <input type='number' name='month' min='1' max='12'></label>"
                  "<label>Year: <input type='number' name='year' min='2020' max='2099'></label>"
                  "<input type='submit' value='Set Time' class='btn time'></form>"
                  "</div>"

                  "</div></body></html>";

    server.send(200, "text/html", html);
}




void handleMove() {
    String direction = server.arg("dir");
    String action = server.arg("action");

    if (action == "start") {
        if (direction == "east" && digitalRead(SENSOR_EAST) == LOW) {
            Serial.println("Moving East");
            digitalWrite(RELAY_EAST, HIGH);
        }
        else if (direction == "west" && digitalRead(SENSOR_WEST) == LOW) {
            Serial.println("Moving West");
            digitalWrite(RELAY_WEST, HIGH);
        }
    } 
    else if (action == "stop") {
        Serial.println("Stopping Motor");
        digitalWrite(RELAY_EAST, LOW);
        digitalWrite(RELAY_WEST, LOW);
    }

    server.send(200, "text/plain", "OK");
}


void handleSettings() {
    autoMode = server.arg("autoMode") == "on";
    morningStartHour = server.arg("morningStart").toInt();
    nightReturnHour = server.arg("nightReturn").toInt();
    stepInterval = server.arg("stepInterval").toInt();

    if (server.hasArg("motorStepTime")) {
        motorStepTime = server.arg("motorStepTime").toInt();
        Serial.print("New Motor Step Time: ");
        Serial.println(motorStepTime);
    }

    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Settings updated");
}




void handleSetTime() {
    RtcDateTime newTime(server.arg("year").toInt(), server.arg("month").toInt(), server.arg("day").toInt(), 
                        server.arg("hour").toInt(), server.arg("minute").toInt(), server.arg("second").toInt());
    Rtc.SetDateTime(newTime);
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Time Updated");
}

// ----------------------- Setup and Loop -----------------------
void setup() {
    Serial.begin(115200);

    pinMode(RELAY_EAST, OUTPUT);
    pinMode(RELAY_WEST, OUTPUT);
    pinMode(SENSOR_EAST, INPUT_PULLDOWN);  // NO: استخدم PULLDOWN
    pinMode(SENSOR_WEST, INPUT_PULLDOWN);  // NO: استخدم PULLDOWN
    digitalWrite(RELAY_EAST, LOW);
    digitalWrite(RELAY_WEST, LOW);

    setupWiFi();
    Rtc.Begin();
    server.on("/", handleRoot);
    server.on("/move", handleMove);
    server.on("/settings", HTTP_POST, handleSettings);
    server.on("/settime", HTTP_POST, handleSetTime);
    server.begin();
}

void loop() {
    server.handleClient(); // تأكد من أنه يُنفذ باستمرار لمعالجة الطلبات

    // الحصول على الوقت الحالي من RTC
    RtcDateTime now = Rtc.GetDateTime();
    int currentHour = now.Hour();

    unsigned long currentMillis = millis();

    if (autoMode) {
        // التحكم بالحركة أثناء النهار
        if (currentHour >= morningStartHour && currentHour < nightReturnHour) {
            returningToEast = false;  // إعادة ضبط حالة العودة
            if (currentMillis - lastMoveTime >= (stepInterval * 60000)) {  // تنفيذ الحركة كل فترة زمنية محددة
                Serial.println("Auto Mode: Moving East");
                moveEast();
                lastMoveTime = currentMillis;
            }
        }

        // العودة إلى الشرق عند حلول الليل
        else if (currentHour >= nightReturnHour && !returningToEast) {
            Serial.println("Auto Mode: Returning to East");
            if (digitalRead(SENSOR_EAST) == LOW) {  
                digitalWrite(RELAY_WEST, HIGH);
                delay(motorStepTime);  // تنفيذ الحركة بفواصل زمنية
                digitalWrite(RELAY_WEST, LOW);
            } else {
                Serial.println("Reached East Position");
                returningToEast = true;  // تأكيد العودة وعدم تكرار العملية
            }
        }
    }
}
