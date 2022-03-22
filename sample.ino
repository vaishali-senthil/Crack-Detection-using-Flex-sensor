/* ESP8266 AWS IoT
 *  
 ----------------------------
 
 ##Compile this only if using ESP8266 version 3+, not backwards comparable
 
 #Tools--> Board--> Board Manager-->ESP8266 version 3+ by ESP8266 Community
 
 ##Older ESP8266 sketch is not forward compatible
 
 --------------------------------
 
 
 * Simplest possible example (that I could come up with) of using an ESP8266 with AWS IoT.
 * No messing with openssl or spiffs just regular pubsub and certificates in string constants
 *
 * This is working as at 3rd Aug 2019 with the current ESP8266 Arduino core release:
 * SDK:2.2.1(cfd48f3)/Core:2.5.2-56-g403001e3=20502056/lwIP:STABLE-2_1_2_RELEASE/glue:1.1-7-g82abda3/BearSSL:6b9587f
 *
 * Author: Anthony Elder
 * License: Apache License v2
 *
 * Sketch Modified by Stephen Borsay for www.udemy.com/course/exploring-aws-iot/
 * https://github.com/sborsay
 * Add in EOF certificate delimiter
 * Add in Char buffer utilizing sprintf to dispatch JSON data to AWS IoT Core
 * First 9 chars of certs obfusicated, use your own, but you can share root CA / x.509 until revoked
 */

 
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
extern "C" {
#include "libb64/cdecode.h"
}

 
const char* ssid = "";  
const char* password = ""; 

const int FLEX_PIN = A0;
const float VCC = 3.29; // Nodemcu Voltage
const float R_DIV = 56000.0; // R2 (external resi value)
const float STRAIGHT_RESISTANCE = 26500.0; // resi when straight
const float BEND_RESISTANCE = 70000.0; // resi at 90 deg

 
// Find this awsEndpoint in the AWS Console: Manage - Things, choose your thing
// choose Interact, its the HTTPS Rest endpoint
const char* awsEndpoint = "a3o7shsfqqm4bq-ats.iot.us-east-1.amazonaws.com"; //your aws iot endpoint
 
// For the two certificate strings below paste in the text of your AWS
// device certificate and private key, comment out the BEGIN and END
// lines, add a quote character at the start of each line and a quote
// and backslash at the end of each line:
 
// xxxxxxxxxx-certificate.pem.crt
const String certificatePemCrt =
R"EOF(MIIDWTCCAkGgAwIBAgIUVoLxWWQ4yjsXeeuGjrwo0AHIWwowDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIxMTAxMTA5NTcw
OFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAK87TEUlmJy1/TRvT/qH
XZu6CwvfnEBoXMHJclYbxCAkH863M+Zo3UqMVY9o9XQ80pJBxxF+Uo0Kji12KbnV
4Lkcai/ZNe9AARXXfyKrv+9yi1HEs0nIb2RrLaFwCiZavCMv+J4d52JsltierKDC
RhsaJ4g0hWLsXL0v73FKOzAGt777TJMPCX9CdJqAL5vAuhvbQPbOIzBYA4XEQXVE
nt91SYj0Y2sOuhzpq9AADjl9SMjPRqkAJ5jd820DaX9X1qeXPx+GpzhDzF4Cemn3
Q6zMLrhPQE+rpp7xcCTg/zxxD2SxnPNTLDnFFPUE984jU9o5HU/eoA5fgVysunMO
l+MCAwEAAaNgMF4wHwYDVR0jBBgwFoAUhR3hM9dvnby7Vq1wc6HYogEtx8kwHQYD
VR0OBBYEFPtD9/Wk67hOw1GwFhTaM+K/h5l+MAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBZMbuFNzq4WHVhmBC9VUGkZLgd
wfLeFdmmpomomrzAUerHibcSG/9nsh8TT7/sJTjz+fEhAtdhM0SizgBHUJtkeFQO
LDCtOKz1YqqrI6fWXbm3xA5JdNLS8OVIDcGFdXg5UM/nRsUCT58LItOoy8RJRmKn
XkAqgvV8Fodrob1ngjmGv2j98oxeap6e02QXlZTsojAkVX07M1x20gc3qX4G5Z1e
p120nKBxdFEpdKTKjUhsr5Rv5ZS9dtpO9kj1jPAQtkN4PWwXfPRwKwbfe8Itub5u
Maks76xocvqYy7TZZb4dANO2w6iKk0zyvldDeSgYseKu7JzH20nA6qHpF+ug)EOF";
 
// xxxxxxxxxx-private.pem.key
const String privatePemKey =
R"EOF(MIIEpAIBAAKCAQEArztMRSWYnLX9NG9P+oddm7oLC9+cQGhcwclyVhvEICQfzrcz
5mjdSoxVj2j1dDzSkkHHEX5SjQqOLXYpudXguRxqL9k170ABFdd/Iqu/73KLUcSz
SchvZGstoXAKJlq8Iy/4nh3nYmyW2J6soMJGGxoniDSFYuxcvS/vcUo7MAa3vvtM
kw8Jf0J0moAvm8C6G9tA9s4jMFgDhcRBdUSe33VJiPRjaw66HOmr0AAOOX1IyM9G
qQAnmN3zbQNpf1fWp5c/H4anOEPMXgJ6afdDrMwuuE9AT6umnvFwJOD/PHEPZLGc
81MsOcUU9QT3ziNT2jkdT96gDl+BXKy6cw6X4wIDAQABAoIBACA2tn6/up6Ulkfd
bsaPvBW0zfMQ2Ji+ls7JScuqrxN5kJ9f7pqdDJF32wLSOM11oQBiN0ZwAp0LI7gX
0PPo7bcaEitZsyCYk5qAU078Y/yRAiu2DX+y6Ud8rJbREgybAZs2Zm4q8S6W5+RK
x1GGZXz2Ae54OU1GRoZBGbOqAdeCFrYDSOCd6kiTaUrFUaHp+vxsiL///5YDPD2T
njc4i+0JI5Y/5PmN3WYQRCSHMPATogOVNeR2kBi1HAS34s2NqC2lTa4cpy0Mue4t
2LGsa0FA6E0OOtyrD3a7AM8hLBidpqEhYRSOjAZCWoyvkwc6xuth2XFpj8vpShoJ
nc7NJoECgYEA3GPwb7IONvMiYkcJD0+nGI3TFY3preqRvilWWa+iNxKQmVp8chYe
6m4bukkVC4kDS9vzl/tvaYYBecYG+2W9RKwV8fcRfVLh95wNv/g/NNQyQ29XO40K
YCBU1PYMAZxAnkKAwtp+N8vDDbCl1K1x/d5ptvwgXi7d/uOqmhxrYu0CgYEAy4ty
PQItIJp0AV2xrZKRXhqfAgGxz5Poj3VAGS2Fx+Tsfo7yb0LbmudKKCkvOqFrqiiK
OjOXGubk38aFR5FDwVar5bvJXV2mW37Q4pI/bsSf1ej1IW0rugwog9l04TMwzEW3
xkCd3eOsVoBU8a/eQG82/TS95EOa4UEsdNAdfA8CgYEAty/yyETN/+q0Z8/3vdxN
XeF01POyirbxEYk2lzGfufsaaf2GdyXf6KhBL+RTM9VdAg7/ORLrlmCmlVt+rjPw
0EXhr8/Xn73myXPTEf/8OaMvbFH9xigDucbl/GXPsP75zxIkCNKoGySpKpXsIQ1S
NNxMLqb/GIoISDdgtNxufjkCgYEAku9xYlVF2nllMO9AKnrZ8cLp8nrir2Sox5bu
1L4mCy1ZA+uRW3w6J9K7l85TR9HrdQSa9HT1qffwVRJseEOHU/SLVuZ+8KWASCB+
DnFg1Uef0r2+54h1vnK0dKnwU/muSmgxyC4xSFLov/EYYPiCSqV+Xr9KLZQYJG/1
9cWe40UCgYBjboakk53fORleAf9uPV01vv9BB4zFKi4sWEdC5HAC7bE/9J1b8NbI
5iPnrpCcmaOb+8XAFZYURO81FPlYEJd5vbADlM8g+etm+iHuramN0ehAUR2l9oyz
JD6iwqIiYnEHbXJ2UNomWnBqU0YTAqY62K3+RpENlPky3xbyGXutgA==)EOF";
 
// This is the AWS IoT CA Certificate from:
// https://docs.aws.amazon.com/iot/latest/developerguide/managing-device-certs.html#server-authentication
// This one in here is the 'RSA 2048 bit key: Amazon Root CA 1' which is valid
// until January 16, 2038 so unless it gets revoked you can leave this as is:
const String caPemCrt = 
R"EOF(MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5)EOF";

 
WiFiClientSecure WiFiClient;

void msgReceived(char* topic, byte* payload, unsigned int len); //function prototype

PubSubClient pubSubClient(awsEndpoint, 8883, msgReceived, WiFiClient);
 
X509List *rootCert;
X509List *clientCert;
PrivateKey *clientKey;
 
 
void setup() {
  Serial.begin(115200); Serial.println();
  pinMode(FLEX_PIN, INPUT);
  Serial.println("ESP8266 AWS IoT Example");
  Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  Serial.print(", WiFi connected, IP address: "); Serial.println(WiFi.localIP());
 
  // get current time, otherwise certificates are flagged as expired
  setCurrentTime();
 
  uint8_t binaryCert[certificatePemCrt.length() * 3 / 4];
  int len = b64decode(certificatePemCrt, binaryCert);
  clientCert = new BearSSL::X509List(binaryCert, len);
 
  uint8_t binaryPrivate[privatePemKey.length() * 3 / 4];
  len = b64decode(privatePemKey, binaryPrivate);
  clientKey = new BearSSL::PrivateKey(binaryPrivate, len);
 
  WiFiClient.setClientRSACert(clientCert, clientKey);
 
  uint8_t binaryCA[caPemCrt.length() * 3 / 4];
  len = b64decode(caPemCrt, binaryCA);
  rootCert = new BearSSL::X509List(binaryCA, len);
 
  WiFiClient.setTrustAnchors(rootCert);
}
 
unsigned long lastPublish;
int msgCount;
 
void loop() {
 
  pubSubCheckConnect();

  //Add a JSON package of fake data to deliver to AWS IoT
  //Uses snprintf but other viable options are: sprintf, strcpy, strncpy, or  
  //Use the ArduinoJson library for Efficient JSON serialization  
  //If you need to increase buffer size, then you need to change MQTT_MAX_PACKET_SIZE in PubSubClient.h
  
   // Read the ADC, and calculate voltage and resistance from it
  int flexADC = analogRead(FLEX_PIN);
  float flexV = flexADC * VCC / 1023.0;//felxADC=Vout
  float flexR = R_DIV * (VCC / flexV - 1.0);
 // Serial.println("Resistance: " + String(flexR) + " ohms");//

  // Use the calculated resistance to estimate the sensor's
  // bend angle:
  //float angle = map(flexR, STRAIGHT_RESISTANCE, BEND_RESISTANCE,
  //                   0, 90.0);   
                   
  char fakeData[128];  
  sprintf(fakeData,  "{\"uptime\":%lu,\"crackdegree\":%.2f}", millis() / 1000, flexR);
 
  if (millis() - lastPublish > 10000) {
     pubSubClient.publish("outTopic", fakeData);  
     Serial.print("Published: "); Serial.println(fakeData);
     lastPublish = millis();
  }
}
 
void msgReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on "); Serial.print(topic); Serial.print(": ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
 
void pubSubCheckConnect() {
  if ( ! pubSubClient.connected()) {
    Serial.print("PubSubClient connecting to: "); Serial.print(awsEndpoint);
    while ( ! pubSubClient.connected()) {
      Serial.print(".");
      pubSubClient.connect("ESPthing");
    }
    Serial.println(" connected");
    pubSubClient.subscribe("inTopic");
  }
  pubSubClient.loop();
}
 
int b64decode(String b64Text, uint8_t* output) {
  base64_decodestate s;
  base64_init_decodestate(&s);
  int cnt = base64_decode_block(b64Text.c_str(), b64Text.length(), (char*)output, &s);
  return cnt;
}
 
void setCurrentTime() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
 
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: "); Serial.print(asctime(&timeinfo));
}
