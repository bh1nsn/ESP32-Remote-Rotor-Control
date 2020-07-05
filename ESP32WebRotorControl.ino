/*--------------------------------------------------------------
 * WiFI Rotor Controller - Version 1.8

  Description:  ESP32 server in combination with ajax calls
                realizes an app which constantly show the bearing
                of a rotor such as the Kenpro 2000 and Yaesu ...
                You can press CW or CCW (ClockWise or 
                Counter Clock Wise) to make the rotor turn.
                Pressing the brake, will first stop any CW or CCW movement
                and then apply the brake after approx 1000 ms (1 sec)
                Turning beyond 359 degrees or less than 0 is
                not possible. The Web app will stop either movement and the dial turns red
                The software does not maintain the position of the antenna. 
                Once you stop the rotor, that is it. If it slips, the value will be shown.
  
  Hardware:     ESP32 Wrover board. Should also work with ESP8266
                boards but needs to refine the pins as well as certain libraries. To be investigated
                
                Used in & outputs
                
                Analogue Input
                To read rotor potentimeter value (use 3 wires)
                34  GPIO36 (ADC1_6 pin 34) Max Voltage is 3.3V - check the calibration !!!!!
                
                Digital inputs
                35  Digital input 1  : Free
                32  Digital input 2  : Free 
                33  Digital input 3  : Free
                25  Digital input 4  : Free

                Outputs
                26  Digital output 1 : CW relais    - using HIGH level as OFF   HL-52S TYPE OF BOARDS
                27  Digital output 1 : CCW relais   - using HIGH level as OFF 
                14  Digital output 1 : BRAKE relais - using HIGH level as OFF 
                12  Digital output 1 : Free
                Relays: Jotta SSR-25 da
                
  Software:     Developed using Arduino 1.8.12 software for the ESP32 Wrover.      
                Should be compatible with Arduino 1.0 + and newer ESP32/Arduino releases if any    
                Internal flash File system contains web pages index.htmL, update.html and some css and js files
                Use ESP32 Tool to upload File system contents via Arduino IDE.
                
  Date:         04-07-2020
 
  Author:       Erik Schott - erik@pa0esh.com
--------------------------------------------------------------*/
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <Update.h>


// Constants

const char* host = "webrotor";
const char *default_instance  = "Web Rotor Controller by PA0ESH";
const char *ssid_wl           = "xxxxxxxxxx";
const char *ssid_ap           = "webrotor";
const char *password          = "xxxxxxxxxx";


const int dns_port  = 53;
const int http_port = 80;
const int ws_port   = 1337;

const int led_pin     = 32;  // Testing LED pin
const int cw_pin      = 26;  // connect your cw relais between GND and this pin
const int ccw_pin     = 27;  // connect your ccw relais between GND and this pin
const int brake_pin   = 14;  // connect your brake relais between GND and this pin
const int spare_pin   = 12;  // connect your spare relais between GND and this pin
const int analog_pin  = 34;  // connect your spare relais between GND and this pin

int led_state     = 0;
int cw_state      = 1;
int ccw_state     = 1;
int brake_state   = 1;
int stop_state    = 1;
int analog_val    = 0;
int analog_val_old = 0;
int StartBearing   = 0;
int graden         = 0;
uint16_t set_azi;
int set_man = 0;

boolean alreadyRun = false;       // check if a fuction has run once
boolean LED_state[4] = {0};       // stores the states of the LEDs
#define Rotor_Msg 10
String results[Rotor_Msg] = { "L manual  CCW", "R manual CW", "A stop rotation", "C current Azimuth" }; // commands to control VERONVRZA digital rotor.
String web_msg;
// Globals

char msg_buf[10];
char *rot_string;
char client_rotor;
String rotor;
String rotor_stop;
String callsign;
String Lat;
String Lon;
String Interface;

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(1337);

/***********************************************************
 * Functions
 */

// Callback: receiving any WebSocket message
void onWebSocketEvent(uint8_t client_num,
                      WStype_t type,
                      uint8_t * payload,
                      size_t length) {
 
  // Figure out the type of WebSocket event
  switch(type) {
 
    // Client has disconnected
    case WStype_DISCONNECTED:
        set_stop();
      Serial.printf("[%u] Disconnected!\n", client_num);
      
      break;
 
    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;
 
    // Handle text messages from client
    case WStype_TEXT:
 
      // Print out raw message
      Serial.printf("Received text from: [%u] Payload: %s\n", client_num, payload);
      // obtaining the bearing set value from the string


 if ( strcmp((char *)payload, "toggleLED") == 0 ) {
        led_state = led_state ? 0 : 1;
        Serial.printf("Toggling LED to %u\n", led_state);
        digitalWrite(led_pin, led_state);

  // Toggle CCW
      } else if ( strcmp((char *)payload, "toggleCCW") == 0 ) {

       if (brake_state == 0){
        exit;
        } else {
        Serial.println("Starting the CCW switch check");
        Serial.printf("CW_State is  %u\n", cw_state);
        Serial.printf("CCW_State is  %u\n", ccw_state);
        if (cw_state == 0){
             cw_state=1;
             digitalWrite(cw_pin, cw_state);
             delay(200);
            }
        Serial.printf("De CCW state was  %u\n", ccw_state);
        ccw_state = !ccw_state;
        Serial.printf("Toggling CCW switch to %u\n", ccw_state);
        digitalWrite(ccw_pin, ccw_state);
        }

// Toggle CW
      } else if ( strcmp((char *)payload, "toggleCW") == 0 ) {
       if (brake_state == 0){
        exit;
        } else {
        Serial.println("Starting the CW switch check");
        Serial.printf("CCW_State is  %u\n", ccw_state);
        Serial.printf("CW_State is  %u\n", cw_state);
        if (ccw_state == 0){
             ccw_state=1;
             digitalWrite(ccw_pin, ccw_state);
             delay(200);
            }
        Serial.printf("De CW state is now  %u\n", cw_state);
        cw_state = !cw_state;
        Serial.printf("Toggling CW switch to %u\n", cw_state);
        
        digitalWrite(cw_pin, cw_state);
         Serial.printf("De CW state is now %u\n", cw_state);
        }

        
   // Toggle BRAKE
      } else if ( strcmp((char *)payload, "toggleBRAKE") == 0 ) {
        brake_state = !brake_state;
      
        Serial.printf("Toggling the Brake to %u\n", brake_state);
        // check if CW and CCW switches are OFF, or switch them OFF

        if (cw_state == 0){
         cw_state = 1;
         set_man = 0;
         Serial.printf("Toggling the CW switch to %u\n", cw_state);
         digitalWrite(cw_pin, cw_state);
         delay(200);
        }

        if (ccw_state == 0){
         ccw_state=1;
         set_man = 0;
         Serial.printf("Toggling the CCW switch to %u\n", ccw_state);
         digitalWrite(ccw_pin, ccw_state);
         delay(200);
        }
  
        digitalWrite(brake_pin, brake_state);

// Report the state of the LED
      } else if ( strcmp((char *)payload, "getLEDState") == 0 ) {
        sprintf(msg_buf, "%d", led_state);
        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        webSocket.broadcastTXT(msg_buf);

// Report the state of the CW button
      } else if ( strcmp((char *)payload, "getCWState") == 0 ) {
        sprintf(msg_buf, "%d", cw_state +2);
        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        webSocket.broadcastTXT(msg_buf);
        
// Report the state of the CCW button
      } else if ( strcmp((char *)payload, "getCCWState") == 0 ) {
        sprintf(msg_buf, "%d", ccw_state+4);
        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        webSocket.broadcastTXT(msg_buf);

// Report the state of the rotor bearing 
      } else if ( strcmp((char *)payload, "getBEARINGState") == 0 ) {
        Serial.println("Sending bearing data to client");
        analog_val = analogRead(analog_pin);   
        analog_val = map(analog_val,4,4096,0,360);  // here is wehere you convert from voltage rotor into degrees
        rotor = String(analog_val);
        rotor = "Bearing :"+rotor,
        webSocket.broadcastTXT( rotor);
        
// Report the state of the BRAKE button
      } else if ( strcmp((char *)payload, "getBRAKEState") == 0 ) {
        sprintf(msg_buf, "%d", brake_state+6);
        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        webSocket.broadcastTXT(msg_buf);

      // Return the WiFi IP number to the client
      } else if ( strcmp((char *)payload, "getWiFiIp") == 0 ) {
      sprintf(msg_buf, "%d", WiFi.localIP()); 
      Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);      
      webSocket.broadcastTXT(msg_buf);


// Return the  configuration items to the client - reading them from spiffs file test.txt
      } else if ( strcmp((char *)payload, "getConfig") == 0 ) {
         Serial.println("getConfig identified !");
         send_config();
        
       
      //Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);      
      //webSocket.broadcastTXT(msg_buf);

 
// Allow the sending of bearing data - client is ready
      } else if ( strcmp((char *)payload, "StartBearing") == 0 ) {
        StartBearing = 1;
 
      // Message not recognized
      } else {
            // Here the manual setting is decoded.
            String str = (char*)payload;

            // split key and value
            String key  = str.substring(0, str.indexOf(':'));
            String value = str.substring(str.indexOf(':') + 1);

            if (brake_state == 1){
            if(key.equalsIgnoreCase("manual")) {
                // convert value to Int
                set_azi= value.toInt();
                Serial.print("Azimuth set value: ");
                Serial.println(set_azi);
                set_man = 1;
             }
            }
      }
      break;
 
    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}


// Callback: send homepage index.html
void onIndexRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/index.html", "text/html");
}

// Callback: send update.html
void onUPDATERequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/update.html", "text/html");
}


// Callback: send css
void onCSSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/bootstrap.min.css", "text/css");
}

// Callback: send css
void onBMMRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/bootstrap.min.css.map", "text/css");
}

// Callback: send css
void onXCRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/xcode.css", "text/css");
}

// Callback: send css
void onZHMRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/ziehharmonika.css", "text/css");
}

// Callback: send css
void onZHJSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/ziehharmonika.js", "application/javascript");
}

// Callback: send javascript standard functions
void onJSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/jquery.js", "application/javascript");
}

// Callback: send javascript gauge meter
void onJGRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/gauge.min.js", "application/javascript");
}

// Callback: send javascript gauge meter
void onBMRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/bootstrap.min.js", "application/javascript");
}

//Callback: send javascript segment-display
void on7SEGRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/segment-display.js", "application/javascript");
}


//Callback: send javascript segment-display
void onHLPRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/highlight.pack.js", "application/javascript");
}


// Callback: send bg pic
void onJPGRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/europe.jpg", "text/plain");
}

// Callback: send bg pic
void onPNGRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/1x1-test_netwerk.png", "text/plain");
}

// Callback: send 404 if requested file does not exist
void onPageNotFound(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Not found");
}

// routine to stop the rotor immedeately
 void set_stop (){
    cw_state = stop_state;
    ccw_state = stop_state;
    digitalWrite(cw_pin, stop_state);
    digitalWrite(ccw_pin, stop_state);
    Serial.print("CW & CCW switches both to OFF - EMERGENCY STOP ");
 }
/***********************************************************
 * Main
 */
 
void setup() {
  // Init LED and turn off
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  pinMode(cw_pin, OUTPUT);
  digitalWrite(cw_pin, HIGH);
  pinMode(ccw_pin, OUTPUT);
  digitalWrite(ccw_pin, HIGH);
  pinMode(brake_pin, OUTPUT);
  digitalWrite(brake_pin, HIGH);
  pinMode(spare_pin, OUTPUT);
  digitalWrite(spare_pin, LOW);

 Serial.println(xPortGetCoreID());
  // Start Serial port
  Serial.begin(115200);

 WiFi.begin(ssid_wl, password);
 
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Connecting...\n\n");
}
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  // Make sure we can read the file system
  if( !SPIFFS.begin()){
    Serial.println("Error mounting SPIFFS");
    while(1);
  }

// begin of spiffs reading
File root = SPIFFS.open("/");
File file = root.openNextFile();
  while(file){
      Serial.print("FILE: ");
      Serial.println(file.name());
      file = root.openNextFile();
  }

// end of spiffs
  if (!MDNS.begin("webrotor")) {
        Serial.println("Error setting up MDNS responder!");
     while(1) {
     delay(1000);
        }
    }
  Serial.println("mDNS responder started with http://webrotor.local");
    
  // Start access point
  WiFi.softAP(ssid_ap, password);
 
  // Print our IP address
  Serial.println();
  Serial.println("AP running");
  Serial.print("My IP address: ");
  Serial.println(WiFi.softAPIP());
 
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/", HTTP_GET, onUPDATERequest);
  server.on("/bootstrap.min.css", HTTP_GET, onCSSRequest);
  server.on("/bootstrap.min.css.map", HTTP_GET, onBMMRequest);
  server.on("/xcode.css", HTTP_GET, onXCRequest);
  server.on("/ziehharmonika.css", HTTP_GET, onZHMRequest);
  server.on("/ziehharmonika.js", HTTP_GET, onZHJSRequest);
  server.on("/highlight.pack.js", HTTP_GET, onHLPRequest);
  server.on("/jquery.js", HTTP_GET, onJSRequest);
  server.on("/gauge.min.js", HTTP_GET, onJGRequest);
  server.on("/bootstrap.min.js", HTTP_GET, onBMRequest);
  server.on("/europe.jpg", HTTP_GET, onJPGRequest);
  server.on("/1x1-test_netwerk.png", HTTP_GET, onPNGRequest);
  server.on("/segment-display.js", HTTP_GET, on7SEGRequest);

  // Handle requests for pages that do not exist
  server.onNotFound(onPageNotFound);


 
  // Start web server
  server.begin();

 MDNS.addService("http", "tcp", 80);
  
  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  
}

void send_config(){
      String config_id;
      Serial.println("getCondif sub routine entered !");
      File file2 = SPIFFS.open("/test.txt", "r");
        if (!file2) {
          Serial.println("Failed to open file for reading");
          return;
        }
        Serial.println("File Content:");
        while (file2.available()) {
        String line = file2.readStringUntil('\n'); // lees het bestand lijn voor lijn.
        if (line.substring(0,3) == "4 -") {
            Serial.println(line.substring(13));
            callsign = line.substring(13);
        }
          if (line.substring(0,3) == "5 -") {
              Serial.println(line.substring(8));
              Lat = line.substring(8);
        }
        if (line.substring(0,3) == "6 -") {
           Serial.println(line.substring(8));
           Lon=line.substring(8);
        }
        if (line.substring(0,3) == "7 -") {
            Serial.println(line.substring(14));
            Interface=line.substring(14);
        }
      }
      config_id  = "Config:"+callsign+" - "+Lat+" - "+Lon+" - "+Interface;
      webSocket.broadcastTXT(config_id);
      Serial.println(config_id);
  
}


// Rotor bearing values taken from 500 ohm potentiometer in rotor itself 
// middle pin connects to 34, one side to GND and one side to 3.3V
void read_rotor_bearing(){
      analog_val = analogRead(analog_pin);   
      graden = map(analog_val,10,4040 ,0,360);  // here is wehere you convert from voltage rotor into degree
      // calibration routine to be be written still
}


void stop_manual(){

 if (cw_state == 0 && set_man ==1 && graden > set_azi) {
               // stop the cw rotation
               cw_state=1;
               digitalWrite(cw_pin, cw_state);
               Serial.printf("De CW state was switched to %u\n", cw_state);
               rotor_stop = String(3);
               webSocket.broadcastTXT(rotor_stop);
               Serial.print("Rotor stopped at : ");
               Serial.println(graden);
               set_man=0;
               delay(500);
  
 }  else if (ccw_state ==0 && set_man ==1 && graden < set_azi+10){
               ccw_state=1;
               digitalWrite(ccw_pin, ccw_state);
               Serial.printf("De CCW state was switched to %u\n", cw_state);
               rotor_stop = String(5);
               webSocket.broadcastTXT(rotor_stop);
               Serial.print("Rotor stopped at : ");
               Serial.println(graden);
               set_man=0;
               delay(500);
  }
 }


// emergency stop routine if rotor get's to end stop values.
void emergency_stop(){
       rotor_stop = String(99);
       if ((analog_val < 100) && (ccw_state == 0) ) {
            ccw_state = 1;
           digitalWrite(ccw_pin, ccw_state);
           rotor_stop = String(5);
           webSocket.broadcastTXT(rotor_stop);
           set_man=0;
           delay(1000);
    } else if (analog_val > 4060 && cw_state == 0 ) {
         rotor_stop = String(3);
         cw_state = 1;
         digitalWrite(cw_pin, cw_state);
         webSocket.broadcastTXT(rotor_stop);
         set_man=0;
         delay(1000);
    } 
    }


// routine to sent continously the bearing values
void sent_bearing_ws(){
     read_rotor_bearing();
     rotor = String(graden);
     rotor = "Bearing :"+rotor,
     webSocket.broadcastTXT(rotor);
     //Serial.print("Sending bearing data to client :");
     //Serial.println(analog_val);
     analog_val_old = analog_val;
     emergency_stop();   
}


//The main loop
void loop() {
// Look for and handle WebSocket data
  webSocket.loop();
  sent_bearing_ws();
  stop_manual();
  
delay(600);  // experimental value, may be incresed or lowered depending on results of page loading and data changes.
}
