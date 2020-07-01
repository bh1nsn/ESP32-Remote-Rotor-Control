/*--------------------------------------------------------------
 * WiFI Rotor Controller - Version 1.6

  Description:  ESP32 server in combination with ajax calls
                realizes an app which constantly show the bearing
                of a rotor such as the Kenpro 2000 and Yaesu ...
                You can press CW or CCW (ClockWise or 
                Counter Clock Wise) to make the rotor turn.
                Pressing the brake, will first stop any CW or CCW movement
                and then apply the brake after approx 1000 ms (1 sec)
                Turning beyond 359 degrees or less than 0 is
                not possible. The Web app will stop either movement and the dial turns red
                The software does not maintain the position
                of the antenna. Once you stop the rotor, that is it. If it slips, the value will be shown.
  
  Hardware:     ESP32 Wrover board. Should also work with ESP8266
                boards but needs to refine the pins as well as certain libraries. To be investigated
                
                Used in & outputs
                
                Aanalogue Input - to read rotor potentimeter value (use 3 wires)
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
                
  Software:     Developed using Arduino 1.8.12 software
                for the ESP32 Wrover
                Should be compatible with Arduino 1.0 + and
                newer ESP32/Arduino releases if any
                Internal flash File system should contain web
                page called /index.htmL and some css and js files
                Use ESP32 Tool to upload File system contents via Arduino IDE.
                
  Date:         29-06-2020
 
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
const char *ssid_wl           = "Kotona-Boven-2.4";
const char *ssid_ap           = "webrotor";
const char *password          =  "Stt1951_mrs";
const char *msg_toggle_led    = "toggleLED";
const char *msg_toggle_CW     = "toggleCW";
const char *msg_toggle_CCW    = "toggleCCW";
const char *msg_toggle_Brake  = "toggleBRAKE";
const char *msg_get_led       = "getLEDState";
const char *msg_get_CW        = "getCWState";
const char *msg_get_CCW       = "getCCWState";
const char *msg_get_Brak      = "getBRAKEState";

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

boolean LED_state[4] = {0};       // stores the states of the LEDs
#define Rotor_Msg 10
String results[Rotor_Msg] = { "L manual  CCW", "R manual CW", "A stop rotation", "C current Azimuth" }; // commands to control VERONVRZA digital rotor.

// Globals

char msg_buf[10];
char *rot_string;
char client_rotor;

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
 
      // Toggle LED
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
        Serial.println("Starting the CCW switch check");
        Serial.printf("CCW_State is  %u\n", cw_state);
        if (ccw_state == 0){
        ccw_state=1;
         digitalWrite(ccw_pin, ccw_state);
         delay(200);
        }
       cw_state = !cw_state;
        Serial.printf("Toggling CW switch to %u\n", cw_state);
        digitalWrite(cw_pin, cw_state);
        }
          // Toggle BRAKE
      } else if ( strcmp((char *)payload, "toggleBRAKE") == 0 ) {
        brake_state = !brake_state;
        Serial.printf("Toggling the Brake to %u\n", brake_state);
        // check if CW and CCW switches are OFF, or switch them OFF

        if (cw_state == 0){
         cw_state=1;
         Serial.printf("Toggling the CW switch to %u\n", cw_state);
         digitalWrite(cw_pin, cw_state);
         delay(200);
        }

        if (ccw_state == 0){
         ccw_state=1;
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

        // Report the state of the rotor bearing button
      } else if ( strcmp((char *)payload, "getBEARINGState") == 0 ) {
        Serial.println("Sending bearing data to client");
        analog_val = analogRead(analog_pin);   
        analog_val = map(analog_val,4,4096,0,360);  // here is wehere you convert from voltage rotor into degrees
        String rotor = String(analog_val);
        rotor = "Bearing :"+rotor,
        webSocket.broadcastTXT( rotor);
        
        // Report the state of the BRAKE button
      } else if ( strcmp((char *)payload, "getBRAKEState") == 0 ) {
        sprintf(msg_buf, "%d", brake_state+6);
        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        webSocket.broadcastTXT(msg_buf);

          // Report the state of the BRAKE button
      } else if ( strcmp((char *)payload, "StartBearing") == 0 ) {
        //sprintf(msg_buf, "%d", brake_state+6);
        //Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        StartBearing = 1;
 
      // Message not recognized
      } else {
        Serial.println("[%u] Message not recognized");
        
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


// Callback: send javascript gauge meter
void onMPRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/connected.mp3", "application/javascript");
}

// Callback: send bulbon and off
void onGIFONRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/pic_bulbon.gif", "text/plain");
}

// Callback: send bulbon and off
void onGIFOFFRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/pic_bulboff.gif", "text/plain");
}

// Callback: send bg pic
void onJPGRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/europe.jpg", "text/plain");
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
    Serial.print("CW & CCW switches to OFF ");
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
 
  // On HTTP request for root, provide index.html file
  server.on("/", HTTP_GET, onIndexRequest);

 // On HTTP request for update.html file
  server.on("/", HTTP_GET, onUPDATERequest);
 
  // On HTTP request for style sheet, provide style.css
  server.on("/bootstrap.min.css", HTTP_GET, onCSSRequest);

// On HTTP request for style sheet, provide style.css
  server.on("/bootstrap.min.css.map", HTTP_GET, onBMMRequest);

// On HTTP request for style sheet, provide style.css
  server.on("/xcode.css", HTTP_GET, onXCRequest);


// On HTTP request for ziehharmonika.css, provide 
  server.on("/ziehharmonika.css", HTTP_GET, onZHMRequest);

  // On HTTP request for ziehharmonika.css, provide 
  server.on("/ziehharmonika.js", HTTP_GET, onZHJSRequest);


// On HTTP request for highlight_pack.js, provide 
  server.on("/highlight.pack.js", HTTP_GET, onHLPRequest);


 // On HTTP request for jquery, provide 
  server.on("/jquery.js", HTTP_GET, onJSRequest);

// On HTTP request for gauge js
  server.on("/gauge.min.js", HTTP_GET, onJGRequest);

// On HTTP request for gauge js
  server.on("/bootstrap.min.js", HTTP_GET, onBMRequest);


// On HTTP request for connect.mp3 js
  server.on("/connected.mp3", HTTP_GET, onMPRequest);

  // On HTTP request for bulbon.gif 
  server.on("/pic_bulbon.gif", HTTP_GET, onGIFONRequest);

  // On HTTP request for bulboff.gif
  server.on("/pic_bulboff.gif", HTTP_GET, onGIFOFFRequest);

// On HTTP request for europe.jpg
  server.on("/europe.jpg", HTTP_GET, onJPGRequest);

  // On HTTP request for 7-segment display
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

// Rotor bearing values taken from 500 ohm potentiometer - middle pin connects to 34
void read_rotor_bearing(){
      analog_val = analogRead(analog_pin);   
      graden = map(analog_val,10,4040 ,0,360);  // here is wehere you convert from voltage rotor into degree
      // calibration routine to be be written still
}

// emergenct stop routine if rotor get's to end stop values.
void emergency_stop(){
       //Serial.print("The STOP value is:");
       //Serial.println(analog_val); 
       //sprintf(msg_buf, "%d", ccw_state);
       //Serial.printf("The CCW state is : %s\n", msg_buf);
        String rotor_stop = String(99);
       if ((analog_val < 60) && (ccw_state == 0) ) {
       rotor_stop = String(99);
       webSocket.broadcastTXT(rotor_stop);
       rotor_stop = String(5);
       ccw_state = 1;
       digitalWrite(ccw_pin, ccw_state);
       webSocket.broadcastTXT(rotor_stop);
       
      delay(500);

    } else if (analog_val > 4000 && cw_state == 0 ) {
       Serial.print("The STOP value is:");
       Serial.println(analog_val); 
       rotor_stop = String(3);
       cw_state = 1;
       digitalWrite(cw_pin, cw_state);
       webSocket.broadcastTXT(rotor_stop);
       rotor_stop = String(99);
       webSocket.broadcastTXT(rotor_stop);
       delay(500);
    }
}

// routine to sent continously the bearing values
void sent_bearing_ws(){
     // compare last & current value of bearing. Only sent new data if > 2 degrees.
     read_rotor_bearing();
     String rotor = String(graden);
     rotor = "Bearing :"+rotor,
     webSocket.broadcastTXT(rotor);
     analog_val_old = analog_val;
     emergency_stop();
}


//The main loop
void loop() {
// Look for and handle WebSocket data
  webSocket.loop();
  sent_bearing_ws();
delay(600);  // experimental value, may be incresed or lowered depending on results of page loading and data changes.
}
