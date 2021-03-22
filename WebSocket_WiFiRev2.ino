/* WebSocket_WiFiRev2
 * Deriviative work from several of the builtin examples.
 * Markus Sattler's websockets library takes up 150% of the memory.
 */

String GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

#include <SPI.h>
#include <WiFiNINA.h>
#include <Arduino_LSM6DS3.h>
#include "cencode_inc.h"
#include "libsha1.h"
#include "webpage.h"

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key index number (needed only for WEP)

int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);
WiFiServer wsserver(8080);
WiFiClient wsclient;

/**
 * base64_encode
 * @param data uint8_t *
 * @param length size_t
 * @return base64 encoded String
 */
String base64_encode(uint8_t * data, size_t length) {
    size_t size   = ((length * 1.6f) + 1);
    char * buffer = (char *)malloc(size);
    if(buffer) {
        base64_encodestate _state;
        base64_init_encodestate(&_state);
        int len = base64_encode_block((const char *)&data[0], length, &buffer[0], &_state);
        len     = base64_encode_blockend((buffer + len), &_state);

        String base64 = String(buffer);
        free(buffer);
        return base64;
    }
    return String("-FAIL-");
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");

  pinMode(led, OUTPUT);      // set the LED pin mode

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // by default the local IP address will be 192.168.4.1
  // you can override it with the following:
  // WiFi.config(IPAddress(10, 0, 0, 1));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  #if IS_ACCESS_POINT
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }
  #else
  status = WiFi.begin(ssid, pass);
  while(status != WL_CONNECTED) {
      delay(100);
      status = WiFi.begin(ssid, pass);
  }
  #endif


  // wait 3 seconds for connection:
  delay(3000);

  IMU.begin();

  // start the web server on port 80
  server.begin();
  wsserver.begin();
  wsclient.stop();

  // you're connected now, so print out the status
  printWiFiStatus();

  WiFiDrv::pinMode(25, OUTPUT);
  WiFiDrv::pinMode(26, OUTPUT);
  WiFiDrv::pinMode(27, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void check_wifi_status()
{
   // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }
}

void check_web_request()
{
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                         // if you get a client,
    Serial.println("new client");       // print a message out the serial port
    String request = "";                // make a String to hold incoming data from the client
    if (client.connected()) {           // loop while the client's connected
      while (client.available()) {      // if there's bytes to read from the client,     
          char c = client.read();       // read a byte, then
          Serial.write(c);              // print it out the serial monitor
          request += c;                 // add it to current
      }
    }
    if (request.startsWith("GET / HTTP/1.1")) {
        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
        // and a content-type so the client knows what's coming, then a blank line:
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        //client.println("<html><head><script>var connection = new WebSocket('ws://'+location.hostname+':8080/');connection.onopen = function () {  connection.send('Connect ' + new Date()); }; connection.onerror = function (error) {    console.log('WebSocket Error ', error);};connection.onmessage = function (e) {  console.log('Server: ', e.data);};function sendRGB() {  var r = parseInt(document.getElementById('r').value).toString(16);  var g = parseInt(document.getElementById('g').value).toString(16);  var b = parseInt(document.getElementById('b').value).toString(16);  if(r.length < 2) { r = '0' + r; }   if(g.length < 2) { g = '0' + g; }   if(b.length < 2) { b = '0' + b; }   var rgb = '#'+r+g+b;    console.log('RGB: ' + rgb); connection.send(rgb); }</script></head><body>LED Control:<br/><br/>R: <input id=\"r\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>G: <input id=\"g\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>B: <input id=\"b\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/></body></html>");
        client.println(webpage);
        // The HTTP response ends with another blank line:
        client.println();
    } else {
        client.println("HTTP/1.1 404 Not Found");
        client.println();     
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void handshake()
{
  size_t matchpos = 0;
  bool nonce_active = false;
  String nonce = "";
  String Sec_WebSocket_Key = "Sec-WebSocket-Key: ";
  Serial.println("new WS client");          // print a message out the serial port
  while (wsclient.available()) {
    char c = wsclient.read();
    if (nonce_active) {
      if (c != '\r' && c != '\n') {
        nonce += c;
      } else {
        nonce_active = false;
      }
    }
    if (c == Sec_WebSocket_Key[matchpos]) {
      matchpos++;
      if (matchpos == Sec_WebSocket_Key.length()) {
        nonce_active = true;
      }
    } else {
      matchpos = 0;
    }
  }
  
  if (nonce.length() > 0) {
    uint8_t sha1HashBin[20] = { 0 };
    String clientKey = nonce;
    clientKey += GUID;

    SHA1_CTX ctx;
    SHA1Init(&ctx);
    SHA1Update(&ctx, (const unsigned char*)clientKey.c_str(), clientKey.length());
    SHA1Final(&sha1HashBin[0], &ctx);

    String key = base64_encode(sha1HashBin, 20);
    key.trim();

    Serial.print("Nonce: \"");
    Serial.print(nonce);
    Serial.print("\" -> \"");
    Serial.print(key);
    Serial.println("\"");

    wsclient.print("HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
    wsclient.print("Upgrade: websocket\r\n");
    wsclient.print("Connection: Upgrade\r\n");
    wsclient.print("Sec-WebSocket-Accept: ");
    wsclient.print(key);
    wsclient.print("\r\n\r\n");
  }
}

enum {
  WS_FIN         = 0x80,
  WS_FIN_SHIFT   = 0x07,
  WS_FR_OP_TXT   = 0x01,
  WS_FR_OP_BIN   = 0x02,
  WS_FR_OP_CLOSE = 0x08,
  WS_FR_OP_PING  = 0x09, // 1001
  WS_FR_OP_PONG  = 0x0A, // 1010
  WS_FR_OP_UNSUPPORTED = 0x0F,
};

uint8_t fromhex(char c)
{
  if ('0' <= c && c <= '9') { return c - '0'; }
  if ('A' <= c && c <= 'F') { return c - 'A' + 10; }
  if ('a' <= c && c <= 'f') { return c - 'a' + 10; }
  return 0;
}

uint8_t R;
uint8_t G;
uint8_t B;
uint8_t LED_value;
float ax = 0.0f;
float ay = 0.0f;
float az = 0.0f;
float gx = 0.0f;
float gy = 0.0f;
float gz = 0.0f;
float temp = 0.0f;
float AX, AY, AZ, GX, GY, GZ, TEMP;

void ws_send()
{
  char c;
  c = WS_FIN | WS_FR_OP_BIN;
  wsclient.write(c);
  c = 32;
  wsclient.write(c);
  wsclient.write(R);
  wsclient.write(G);
  wsclient.write(B);
  wsclient.write(LED_value);
  wsclient.write((const char*)&AX, 4);
  wsclient.write((const char*)&AY, 4);
  wsclient.write((const char*)&AZ, 4);
  wsclient.write((const char*)&GX, 4);
  wsclient.write((const char*)&GY, 4);
  wsclient.write((const char*)&GZ, 4);
  wsclient.write((const char*)&TEMP, 4);
}

void check_ws_request()
{
  if (wsclient.connected()) {

    int total = 0;
    while (wsclient.available()) {
      char c = wsclient.read();
      total++;
      char n;
      char op = c & 0x0F;
      Serial.print(c, HEX);
      Serial.print(" ");
      switch(op)
      {
        case WS_FR_OP_TXT: Serial.println("TXT"); break;
        case WS_FR_OP_BIN: Serial.println("BIN"); break;
        case WS_FR_OP_CLOSE: Serial.println("CLOSE"); break;
        case WS_FR_OP_PING: Serial.println("PING"); break;
        case WS_FR_OP_PONG: Serial.println("PONG"); break;
        case WS_FR_OP_UNSUPPORTED: Serial.println("UNSUPPORTED"); break;
      }

      n = wsclient.read();
      bool is_mask = (0x80 & n) != 0;
      n = n & 0x7F;
      total++;
      Serial.println(n, DEC);
      char mask[4];
      mask[0] = wsclient.read();
      mask[1] = wsclient.read();
      mask[2] = wsclient.read();
      mask[3] = wsclient.read();
      String cmd = "";
      while(n > 0) {
        for (char i = 0; i < 4 && n > 0; i++, n--) {
          c = wsclient.read() ^ mask[i];
          cmd += c;
          total++;
        }
      }

      const char* s = cmd.c_str();
      LED_value = fromhex(s[0]);
      R = fromhex(s[2]) << 4 | fromhex(s[3]);
      G = fromhex(s[4]) << 4 | fromhex(s[5]);
      B = fromhex(s[6]) << 4 | fromhex(s[7]); 
      WiFiDrv::analogWrite(25, R);
      WiFiDrv::analogWrite(26, G);
      WiFiDrv::analogWrite(27, B);
      digitalWrite(LED_BUILTIN, LED_value);

      if (wsclient.available()) { readIMU(); continue; }
      
      ws_send();

    }
  } else { 
    wsclient = wsserver.available();
    if (!wsclient.connected()) { wsclient.stop(); return; }
    handshake();
  }

}


int read_count = 0;

void readIMU()
{
  float Ax, Ay, Az;
  float Gx, Gy, Gz;
  float Temp;
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable() && IMU.temperatureAvailable()) {
    
    IMU.readAcceleration(Ax, Ay, Az);
    IMU.readGyroscope(Gx, Gy, Gz);
    IMU.readTemperature(Temp);
    
    ax += Ax;
    ay += Ay;
    az += Az;
    gx += Gx;
    gy += Gy;
    gz += Gz;
    temp += Temp;
    read_count++;
  }
  
  if (read_count == 100) {
    AX = ax / 100.0f;
    AY = ay / 100.0f;
    AZ = az / 100.0f;
    GX = gx / 100.0f;
    GY = gy / 100.0f;
    GZ = gz / 100.0f;
    TEMP = temp / 100.0f;
    ws_send();
    ax = 0.0f;
    ay = 0.0f;
    az = 0.0f;
    gx = 0.0f;
    gy = 0.0f;
    gz = 0.0f;
    temp = 0.0f;
    read_count = 0;
  }
}

void loop()
{

  if (!wsclient.connected()) {
      check_wifi_status();
      check_web_request();
  } else {
      readIMU();
  }
  check_ws_request();
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}
