#include <WiFiNINA.h>
#include <utility/wifi_drv.h>

#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "arduinosecrets.h"
#include "HeaterControl.h"

//uint8_t my_MAC[] = { 0xB0, 0xB2, 0x1C, 0x59, 0x10, 0x14};
uint8_t my_MAC[] = { 0x24, 0x4C, 0x59, 0x1c, 0xb2, 0xb0};

#define GREEN_LED 26
#define RED_LED 25
#define BLUE_LED 27

bool no_wifi_hardware = 0;
bool no_connection = 0;



HeaterControl heat_control;
char ssid[] = SECRET_SSID;       //  your network SSID (name) between the " "
char pass[] = SECRET_PASS;  // your network password between the " "
int keyIndex = 0;             // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;  //connection status
WiFiServer server(80);        //server socket

WiFiClient client = server.available();
const int SENSOR_PIN = 16;
OneWire oneWire(SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
WiFiUDP Udp;
unsigned int localPort = 49000;
//IPAddress remoteIP(192, 168, 0, 105);
IPAddress remote_IP;

unsigned int remote_port = 0 ;

bool T123;
float tempCelsius, T1, T2, T3;

int ledPin = 2;
int buzzerPin = 19;

void setup() {

  //remote_IP = IPAddress(0);

  WiFiDrv::pinMode(GREEN_LED, OUTPUT); //define GREEN LED
  WiFiDrv::pinMode(RED_LED, OUTPUT); //define RED LED
  WiFiDrv::pinMode(BLUE_LED, OUTPUT); //define BLUE LED
  RGBLed(255, 0, 0);
  delay(2000);
  RGBLed(0, 255, 0);
  delay(2000);
  RGBLed(0, 0, 255);
  delay(2000);
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  //pinMode(ledPin, OUTPUT);
  //while (!Serial)
  //  ;
  uint8_t mac[6];
  enable_WiFi();
  connect_WiFi();
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);

  pinMode(A2, INPUT);

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH);
  delay(5000);
  digitalWrite(FAN_PIN, LOW);

#if 0
  for (int i = 0; i < 256; i += 20) {

    analogWrite(FAN_PIN, i);
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
    delay(2000);
  }

for (int i = 0; i < 256; i++) {
    analogWrite(FAN_PIN, 255 - i);
    delay(100);
  }
#endif
  digitalWrite(FAN_PIN, LOW);
  T123 = (memcmp(mac, my_MAC, 6) == 0);
  server.begin();

  Udp.begin(localPort);
  printWifiStatus();
  tempSensor.begin();
  tempSensor.setResolution(12);
}

void loop() {
  //RGBLed(0, 0, 255);
  checkWiFi();
  checkUDP();
  
  static unsigned long counter = 0;
#if 1

  float floor_temp;
  float temp_derivative;
  int voltage_in;
  float voltage;
  static float voltage_flt = 12.5;
  HEATER_STATES heater_state;

  tempSensor.requestTemperatures();
  tempCelsius = tempSensor.getTempCByIndex(0);
  T1 = tempCelsius;  // read temperature in Celsius
  delay(100);
  T2 = tempSensor.getTempCByIndex(1);
  delay(100);
  T3 = tempSensor.getTempCByIndex(2);

  client = server.available();


  if (client) {
    printWEB();
  }
#endif

  if (T123) {
      heat_control.iterate(T2, T3);
  }
  //Udp.beginPacket(WiFi.localIP(),  localPort);
  if (remote_IP && remote_port) {
    Udp.beginPacket(remote_IP, remote_port);
    char response[64];
    if (T123) {
       float time_left;
       int temp_diff_cnt;
 
       heat_control.exportState(floor_temp, temp_derivative, heater_state, time_left, temp_diff_cnt);
       voltage_in = analogRead(A2);
       voltage = 3.3 * (voltage_in / 1024.) * 12.08 / 2.30  + 0.75;
       voltage_flt = 0.90 * voltage_flt + 0.1 * voltage;
      int hs = heater_state + 1;
       sprintf(response, "Voltage %f Voltage_FLT %f Voltage_in %i TFloor %f T1 %f T2 %f T3 %f TDer %f HS %i %TDC %i TimeL %f", voltage, voltage_flt, voltage_in, floor_temp, T1, T2, T3, temp_derivative, (int) heater_state, temp_diff_cnt, time_left);
 
      //Serial.println(hs);
      int b = 255 * (hs &1);
      hs = hs >> 1;
      //Serial.println(hs);
      int g = 255 * (hs &1);
      hs = hs >> 1;
      //Serial.println(hs);
      int r = 255 * (hs & 1);
      // RGBLed(r, g, b);

    } else {
      sprintf(response, "T4 %f T5 %f T6 %f", T1, T2, T3);
    }
      
    Udp.write(response);
    Udp.endPacket();
    //Serial.println(counter);
  }
  //RGBLed(255, 255, 0);
  delay(500);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  //RGBLed(0, 255, 0);
}

void enable_WiFi() {
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    no_wifi_hardware = true;
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }
}

void connect_WiFi() {
  if (no_wifi_hardware)
      return;
  RGBLed(255, 0, 0);
  // attempt to connect to Wifi network:
  status = 0;
  int retries = 5;
  while (status != WL_CONNECTED) {
    RGBLed(255, 0, 0);
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
    if (retries-- == 0) {
      no_connection = true;
      break;
    }
  }
  RGBLed(255, 255, 0);
}

void printWEB() {

  if (client) {                    // if you get a client,
    Serial.println("new client");  // print a message out the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected()) {   // loop while the client's connected
      if (client.available()) {    // if there's bytes to read from the client,
        char c = client.read();    // read a byte, then
        Serial.write(c);           // print it out the serial monitor
        if (c == '\n') {           // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {

            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            //create the buttons
            client.print("Click <a href=\"/H\">here</a> turn the LED on<br>");
            client.print("Click <a href=\"/L\">here</a> turn the LED off<br><br>");

            //int randomReading = analogRead(A1);
            client.print("Temperature: ");
            client.print(tempCelsius);




            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void checkUDP() {
  Serial.print(".");
  while (Udp.parsePacket()) {
    char packet_buffer[255];
    if (Udp.read(packet_buffer, sizeof(packet_buffer))) {
        Serial.print("+");
      if (strncmp(packet_buffer, "HELLO", 5) == 0) {
        Serial.println("Responding to UDP Hello");
        remote_IP = Udp.remoteIP();
        remote_port = Udp.remotePort();
        RGBLed(16, 16, 0);
        delay(500);
        RGBLed(0, 255, 0);


      }
      if (strncmp(packet_buffer, "CMD", 3) == 0) {
        Serial.println("Responding to UDP CMD");
        heat_control.thermostatCmd(packet_buffer);
        //remote_IP = Udp.remoteIP();
        //remote_port = Udp.remotePort();
      }
    }
  }
}

void checkWiFi()
{
 // RGBLed(255, 0, 0);
  if (WiFi.status() != WL_CONNECTED) {
    RGBLed(255, 0, 0);
    Serial.println("***** Restarting WiFi ******");
    Udp.stop();
    WiFi.end();
    enable_WiFi();
    connect_WiFi();
    Udp.begin(localPort);
    printWifiStatus();
    delay(5000);
  }
 // RGBLed(0, 16, 0);
}

void RGBLed(int r, int g, int b)
{
  WiFiDrv::analogWrite(RED_LED, r);
  WiFiDrv::analogWrite(GREEN_LED, g);
  WiFiDrv::analogWrite(BLUE_LED, b);
}
    