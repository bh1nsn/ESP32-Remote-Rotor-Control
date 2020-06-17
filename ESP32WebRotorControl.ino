/*--------------------------------------------------------------
 * WiFI Rotor Controller - test versie

  Description:  ESP32 server in combination with ajax calls
                realizes an app which constantly show the bearing
                of a rotor such as the Kenpro 2000 and Yaesu ...
                You can press CW or CCW (ClockWise or 
                Counter Clock Wise) to make the rotor turn.
                Pressing the brake, will first stop any CW or CCW movement
                and then apply the brake after approx 1000 ms (1 sec)
                Turning beyond 359 degrees or less than 0 is
                not possible.
                The software does not maintain the position
                of the antenna. Once you stop the rotor, that is it.
  
  Hardware:     ESP32 Wrover board. Should also work with other ESP8266
                boards. To be investigated
                Used in & outputs
                Aanalogue Input - to read rotor potentimeter value (use 3 wires)
                34  GPIO36 (ADC1_6 pin 34) Max Voltage is 3.3V - check the calibration !!!!!
                
                Digital inputs
                35  Digital input 1  : Free
                32  Digital input 2  : Free  (is also Led_pin var)
                33  Digital input 3  : Free
                25  Digital input 4  : Free
                26  Digital output 1 : CW relais
                27  Digital output 1 : CCW relais
                14  Digital output 1 : BRAKE relais
                12  Digital output 1 : free
                Relays: Jotta SSR-25 da
                
  Software:     Developed using Arduino 1.8.12 software
                ESP32 WROOM/Arduino 2.3.0
                Should be compatible with Arduino 1.0 + and
                newer ESP32/Arduino releases if any
                Internal flash File system should contain web
                page called /index.htmL. Use ESP32 Tool
                to upload File system contents via Arduino IDE.
                
  Date:         16-06-2020
 
  Author:       Erik Schott - erik@pa0esh.com
--------------------------------------------------------------*/
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

// Constants



const char *ssid_wl           = "Kotona-Boven-2.4";
const char *ssid_ap           = "Webrotor";
const char *password          =  "Stt1951_mrs";
const char *msg_toggle_led    = "toggleLED";
const char *msg_toggle_CW     = "toggleCW";
const char *msg_toggle_CCW    = "toggleCCW";
const char *msg_toggle_Brake  = "toggleBRAKE";
const char *msg_get_led       = "getLEDState";
const char *msg_get_CW        = "getCWState";
const char *msg_get_CCW       = "getCCWState";
const char *msg_get_Brak     = "getBRAKEState";



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
int cw_state      = 0;
int ccw_state     = 0;
int brake_state   = 0;
int stop_state    = 0;
int analog_val    = 0;
int analog_val_old = 0;


boolean LED_state[4] = {0};       // stores the states of the LEDs

 
// Globals
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(1337);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient * globalClient = NULL;
char msg_buf[10];
char *rot_string;

 
/***********************************************************
 * Functions
 */

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
 
  if(type == WS_EVT_CONNECT){
 
    Serial.println("Websocket client connection received");
    globalClient = client;
 
  } else if(type == WS_EVT_DISCONNECT){
 
    Serial.println("Websocket client connection finished");
    globalClient = NULL;
 
  }
}


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
       
    // Toggle CW
      } else if ( strcmp((char *)payload, "toggleCW") == 0 ) {

        if (brake_state == 1){
        exit;
        } else {    
        cw_state = cw_state ? 0 : 1;
        Serial.printf("Toggling CW switch to %u\n", cw_state);
        digitalWrite(cw_pin, cw_state);
        }

  // Toggle CCW
      } else if ( strcmp((char *)payload, "toggleCCW") == 0 ) {

       if (brake_state == 1){
        exit;
        } else {

        ccw_state = ccw_state ? 0 : 1;
        Serial.printf("Toggling CCW switch to %u\n", ccw_state);
        digitalWrite(ccw_pin, ccw_state);
        }

          // Toggle BRAKE
      } else if ( strcmp((char *)payload, "toggleBRAKE") == 0 ) {
        brake_state = brake_state ? 0 : 1;
        Serial.printf("Toggling the Brake to %u\n", brake_state);
        // check if CW and CCW switches are OFF, or switch them OFF

        if (cw_state == 1){
         cw_state=0;
         Serial.printf("Toggling the CW switch to %u\n", cw_state);
         digitalWrite(cw_pin, cw_state);
         delay(2000);
        }

        if (ccw_state == 1){
         ccw_state=0;
         Serial.printf("Toggling the CCW switch to %u\n", ccw_state);
         digitalWrite(ccw_pin, ccw_state);
         delay(2000);
        }
  
        digitalWrite(brake_pin, brake_state);

      // Report the state of the LED
      } else if ( strcmp((char *)payload, "getLEDState") == 0 ) {
        sprintf(msg_buf, "%d", led_state);
        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        webSocket.sendTXT(client_num, msg_buf);

      // Report the state of the CW button
      } else if ( strcmp((char *)payload, "getCWState") == 0 ) {
        sprintf(msg_buf, "%d", cw_state +2);
        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        webSocket.sendTXT(client_num, msg_buf);
        
        // Report the state of the CCW button
      } else if ( strcmp((char *)payload, "getCCWState") == 0 ) {
        sprintf(msg_buf, "%d", ccw_state+4);
        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        webSocket.sendTXT(client_num, msg_buf);
        
        // Report the state of the BRAKE button
      } else if ( strcmp((char *)payload, "getBRAKEState") == 0 ) {
        sprintf(msg_buf, "%d", brake_state+6);
        Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
        webSocket.sendTXT(client_num, msg_buf);
 
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

// Callback: send css
void onCSSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/bootstrap.min.css", "text/css");
}

// Callback: send javascript standard functions
void onJSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/jquery.slim.min.js", "application/javascript");
}

// Callback: send javascript gauge meter
void onJGRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/gauge.min.js", "application/javascript");
}
 
// Callback: send 404 if requested file does not exist
void onPageNotFound(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Not found");
}


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
  digitalWrite(cw_pin, LOW);
  pinMode(ccw_pin, OUTPUT);
  digitalWrite(ccw_pin, LOW);
  pinMode(brake_pin, OUTPUT);
  digitalWrite(brake_pin, LOW);
  pinMode(spare_pin, OUTPUT);
  digitalWrite(spare_pin, LOW);
 
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
 
  // Start access point
  WiFi.softAP(ssid_ap, password);
 
  // Print our IP address
  Serial.println();
  Serial.println("AP running");
  Serial.print("My IP address: ");
  Serial.println(WiFi.softAPIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

 
  // On HTTP request for root, provide index.html file
  server.on("/", HTTP_GET, onIndexRequest);
 

  // On HTTP request for style sheet, provide style.css
  server.on("/bootstrap.min.css", HTTP_GET, onCSSRequest);

 // On HTTP request for jquery, provide 
  server.on("/jquery.slim.min.js", HTTP_GET, onJSRequest);

// On HTTP request for gauge js
  server.on("/gauge.min.js", HTTP_GET, onJGRequest);

  // Handle requests for pages that do not exist
  server.onNotFound(onPageNotFound);
 
  // Start web server
  server.begin();
 
  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  
}

// Rotor bearing values taken from 500 ohm potentiometer - middle pin connects to 34
String xmlResponse()
{
    String res = "";                    // String to assemble XML response values
    int analog_val;                     // stores value read from analog inputs
     
    res += "<?xml version = \"1.0\" ?>\n";
    res +="<inputs>\n";
    // read analog input
    // ESP32 has many Analog inputs (ADC1 - do not use ADC2). It's 12bit (4096 values) Measuring range is 0-3.3V. Beginning and endingis not lineair.
    // use 2 resistors opr potentiometers to calibrate the hardware.
        analog_val = analogRead(analog_pin);   
        analog_val = map(analog_val,4,4096,0,360);  // here is wehere you convert from voltage rotor into degrees
        Serial.println("reading pin 34 as : ");   // then adapt the map values accordingly.
        Serial.print (analog_val);
     

        res += "<analog>";
        res += String(analog_val);
        res += "</analog>\n";
    // read switches
    res += "</inputs>";
    return res;
}

void ajaxInputs() {
  //server.sendHeader("Connection", "close");                         // Headers to free connection ASAP and 
  //server.sendHeader("Cache-Control", "no-store, must-revalidate");  // Don't cache response
  //server.send(200, "text/xml", xmlResponse());                      // Send string from xmlResponse() as XML document to cliend.
                                                                    // 200 - means Success html result code
}

void sent_bearing(){
     // compare last & current value of bearing. Only sent new data if > 3 degrees.
      analog_val = analogRead(analog_pin);   
      analog_val = map(analog_val,4,4096,0,360);  // here is wehere you convert from voltage rotor into degrees
      if (analog_val_old < analog_val-3 || analog_val_old > analog_val +3) {
      String randomNumber = String(random(0,20));
      String rotor = String(analog_val);
      if(globalClient != NULL){
              globalClient->text(rotor);
              Serial.print("Current Rotor bearing is :");
              Serial.println(analog_val);  
              Serial.print("Previous Rotor bearing is :");
              Serial.println(analog_val_old);  
              analog_val_old = analog_val;
                    
      
      }
  }
}
void loop() {
  // Look for and handle WebSocket data
  webSocket.loop();
  sent_bearing();

 delay(500);
}
