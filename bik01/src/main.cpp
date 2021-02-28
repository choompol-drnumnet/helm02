#include <Arduino.h>

#include <WiFi.h>

#include "BLEDevice.h"
#include "MPU9250.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>

#define RXD2 36
#define TXD2 34

#define MAX_RECORD 100

const char* bike_ssid = "BIKE_WIFI1";
const char* bike_pass = "0123456789";

static BLEUUID helmServUUID("4fafc201-1fb5-459e-8fcc-c5c9c3319001");
static BLEUUID helmCharUUID("beb5483e-36e1-4688-b7f5-ea07361b2001");

static BLEUUID jackServUUID("4fafc201-1fb5-459e-8fcc-c5c9c3319002");
static BLEUUID jackCharUUID("beb5483e-36e1-4688-b7f5-ea07361b2002");

WiFiServer server(80);

HardwareSerial SerialGPS(1);

#define I2C_CLK 18
#define I2C_SDA 17

//MPU9250 IMU(Wire,0x68);
int status;

TinyGPSPlus gps;

//#include "BLEScan.h"

#define LED_GREEN 33
#define LED_YELLO 27
#define LED_RED 12

//===========
int gps_v;
double gps_lat, gps_lng;
uint8_t gps_hour, gps_minute, gps_second;
uint32_t gps_time;
int g_sw1, g_cnt, g_alc;
int j_sw1, j_cnt, j_alc;

typedef struct {
  int gps_v;
  double gps_lat, gps_lng;
  uint8_t gps_hour, gps_minute, gps_second;
  uint32_t gps_time;
  int g_on, g_sw1, g_cnt, g_alc;
  int j_on, j_sw1, j_cnt, j_alc;
} record_type;

int waitCnt = 10;
int maxWait = 10;

record_type records[MAX_RECORD];

uint32_t timeCnt = -1;
uint32_t lastTime = 0;
uint32_t diftim = 500;

static boolean doHelmConnect = false;
static boolean helmConnected = false;
static boolean doHelmScan = false;
static BLERemoteCharacteristic* helmChar;
static BLEAdvertisedDevice* helmDevice;
static BLEClient* pHelmClient = NULL;

static boolean doJackConnect = false;
static boolean jackConnected = false;
static boolean doJackScan = false;
static BLERemoteCharacteristic* jackChar;
static BLEAdvertisedDevice* jackDevice;
static BLEClient* pJackClient = NULL;

class HelmCallback : public BLEClientCallbacks {
	void onConnect(BLEClient* pclient) { }
	void onDisconnect(BLEClient* pclient) {
		if(pclient==pHelmClient) {
			ESP.restart();
   			helmConnected = false;
			doHelmScan = true;
			Serial.println("onDisconnect Helmet");
		}
	}
};

class JackCallback : public BLEClientCallbacks {
	void onConnect(BLEClient* pclient) { }
	void onDisconnect(BLEClient* pclient) {
		//ESP.restart();
		if(pclient==pJackClient) {
			ESP.restart();
			jackConnected = false;
			doJackScan = true;
			Serial.println("onDisconnect Jacket");
		}
	}
};

String header;
char buf[100];

bool connHelmServ() {
    Serial.print("Forming a connection to ");
    Serial.println(helmDevice->getAddress().toString().c_str());
    //BLEClient*  pClient  = BLEDevice::createClient();
    pHelmClient  = BLEDevice::createClient();
    Serial.println(" - Created client");
    pHelmClient->setClientCallbacks(new HelmCallback());

    pHelmClient->connect(helmDevice);
    Serial.println(" - Connected to server");

    BLERemoteService* pRemoteService = pHelmClient->getService(helmServUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our helmServUUID UUID: ");
      Serial.println(helmServUUID.toString().c_str());
      pHelmClient->disconnect();
      return false;
    }
    Serial.println(" - Found our helmServUUID");

    helmChar = pRemoteService->getCharacteristic(helmCharUUID);
    if (helmChar == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(helmCharUUID.toString().c_str());
      pHelmClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");
    if(helmChar->canRead()) {
      std::string value = helmChar->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }
    helmConnected = true;
    return true;
}

bool connJackServ() {
    Serial.print("Forming a connection to ");
    Serial.println(jackDevice->getAddress().toString().c_str());
    pJackClient  = BLEDevice::createClient();
    Serial.println(" - Created jacket client");
    pJackClient->setClientCallbacks(new JackCallback());

    pJackClient->connect(jackDevice);
    Serial.println(" - Connected to jacket server");

    BLERemoteService* pRemoteService = pJackClient->getService(jackServUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find jackServUUID UUID: ");
      Serial.println(jackServUUID.toString().c_str());
      pJackClient->disconnect();
      return false;
    }
    Serial.println(" - Found our jackServUUID");

    jackChar = pRemoteService->getCharacteristic(jackCharUUID);
    if (jackChar == nullptr) {
      Serial.print("Failed to find jacket characteristic UUID: ");
      Serial.println(jackCharUUID.toString().c_str());
      pJackClient->disconnect();
      return false;
    }
    Serial.println(" - Found jack characteristic");
    if(jackChar->canRead()) {
      std::string value = jackChar->readValue();
      Serial.print("The jack characteristic value was: ");
      Serial.println(value.c_str());
    }
    jackConnected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advDev) {
    if (advDev.haveServiceUUID() && advDev.isAdvertisingService(helmServUUID)) {
      //BLEDevice::getScan()->stop();
		Serial.println("Helmet Adv");
		helmDevice = new BLEAdvertisedDevice(advDev);
		doHelmConnect = true;
		doHelmScan = true;
    }
    if (advDev.haveServiceUUID() && advDev.isAdvertisingService(jackServUUID)) {
		Serial.println("Jacket Adv");
		jackDevice = new BLEAdvertisedDevice(advDev);
		doJackConnect = true;
		doJackScan = true;
	}
  }
};

void BLEinit() {
	BLEDevice::init("");
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setInterval(1349);
	pBLEScan->setWindow(449);
	pBLEScan->setActiveScan(true);
	pBLEScan->start(5, false);
}

int cntUnfinish = 0;
int cntUnfinMax = 40;

void BLEloop() {
	if (doHelmConnect) {
		if (connHelmServ()) {
			Serial.println("We are now connected to the BLE Server.");
		} else {
			Serial.println("We have failed to connect to helmet server.");
		}
		doHelmConnect = false;
	}
	if (helmConnected) {
		String newValue = "Time since boot: " + String(millis()/1000);
		std::string value_to_be_read = helmChar->readValue();
		const char * pc = value_to_be_read.c_str();
		if(pc[0]==0x53 && pc[1]==0x46 && pc[2]==0x54) {
			sscanf(pc+4, "%d%d%d", &g_cnt, &g_sw1, &g_alc);
			waitCnt = 0;
            sprintf(buf, "HELM: CNT:%d SW:%d ALC:%d", g_cnt, g_sw1, g_alc);
			Serial.println(buf);
		} else {
			Serial.println("=== NG");
			waitCnt = 100;
		}
	} else if(doHelmScan){
		Serial.println("SCAN AFTER disconnect HELMET");
		BLEDevice::getScan()->start(3,false);
		// this is just example to start scan after disconnect
		//, most likely there is better way to do it in arduino
	}
//	Serial.println("... LOOP 1.3");
	if (doJackConnect) {
		if (connJackServ()) {
			Serial.println("We are now connected to the Jacket Server.");
		} else {
			Serial.println("We have failed to connect to jacket server.");
		}
		doJackConnect = false;
	}

	if (jackConnected) {
		std::string value_to_be_read = jackChar->readValue();
		const char * pc = value_to_be_read.c_str();
		if(pc[0]==0x53 && pc[1]==0x46 && pc[2]==0x54) {
			sscanf(pc+4, "%d%d%d", &j_cnt, &j_sw1, &j_alc);
			waitCnt = 0;
            sprintf(buf, "JACK: CNT:%d SW:%d ALC:%d", j_cnt, j_sw1, j_alc);
			Serial.println(buf);
		} else {
			Serial.println("=== NG");
			waitCnt = 100;
		}
	} else if(doJackScan){
		Serial.println("SCAN AFTER disconnect JACKET");
		BLEDevice::getScan()->start(3,false);
		//BLEDevice::getScan()->start(0);
		// this is just example to start scan after disconnect
		//, most likely there is better way to do it in arduino
	}
	if(helmConnected && jackConnected) {
		cntUnfinish = 0;
	} else {
		cntUnfinish++;
		Serial.print(cntUnfinish);
		Serial.println(" Unfinish Times");
		if(cntUnfinish>cntUnfinMax) {
			ESP.restart();
		}
	}

	if(g_sw1==0 && j_sw1==0) {
Serial.println("10: ON");
		digitalWrite(15, 1);	
	} else {
Serial.println("10: OFF");
		digitalWrite(15, 0);
	}
}

void setup() {
	Serial.begin(115200);
	SerialGPS.begin(9600, SERIAL_8N1, RXD2, TXD2);
	Serial.println("Starting Arduino BLE Client application...");

	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_YELLO, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(15, OUTPUT);

	BLEinit();

	WiFi.softAP(bike_ssid, bike_pass);
	IPAddress IP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(IP);
	server.begin();

}

void loop() {
	Serial.println("======= LOOP =======");

	while (SerialGPS.available() > 0) {
		Serial.print(".");
		gps.encode(SerialGPS.read());
	}
	Serial.println();
	gps_v = gps.satellites.value();
	gps_lat = gps.location.lat();
	gps_lng = gps.location.lng();
	gps_time = gps.time.value();
	gps_hour = gps.time.hour()+7;
	if(gps_hour>=24) gps_hour = gps_hour - 24;
	gps_minute = gps.time.minute();
	gps_second = gps.time.second();
	waitCnt++;

	record_type* prec = NULL;
	Serial.println(gps_time);
	if(timeCnt<0) {
 		timeCnt = 0;
		lastTime = gps_time;
		prec = &records[timeCnt];
	} else {
		if(gps_time-lastTime>=diftim) {
			timeCnt++;
			prec = &records[timeCnt%MAX_RECORD];
			lastTime = gps_time;
		}
	}
	if(prec!=NULL) {
		prec->gps_lat = gps_lat;
		prec->gps_lng = gps_lng;
		prec->gps_hour = gps_hour;
		prec->gps_minute = gps_minute;
		prec->gps_second = gps_second;
		prec->gps_time = gps_time;
		prec->g_on = (helmConnected? 1:0);
		prec->g_sw1 = g_sw1;
		prec->g_alc = g_alc;
		prec->j_on = (jackConnected? 1:0);
		prec->j_sw1 = j_sw1;
 	}
//	Serial.println("... LOOP 1");
	BLEloop();
//	Serial.println("... LOOP 2");

	if(waitCnt<maxWait) {
		digitalWrite(LED_YELLO, 1);
	} else {
		digitalWrite(LED_YELLO, 0);
	}
	if(g_sw1==0) {
		digitalWrite(LED_GREEN, 1);
	} else {
		digitalWrite(LED_GREEN, 0);
	}
	if(g_alc>900) {
		digitalWrite(LED_RED, 1);
	} else {
		digitalWrite(LED_RED, 0);
	}

	WiFiClient client = server.available();
	if (client) {
		Serial.println("New Client.");
		String currentLine = "";
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();
				//Serial.write(c);
				header += c;
				if (c == '\n') {
					if (currentLine.length() == 0) {
							//Serial.println(header);
						if (header.indexOf("GET /show") >= 0) {
							client.println("HTTP/1.1 200 OK");
							client.println("Content-type:text/html");
							client.println("Connection: close");
							client.println();
							client.println("<!DOCTYPE html><html lang=\"th\">");
							client.println("<head><meta http-equiv=\"refresh\" content=\"2\"><meta charset=\"UTF-8\"></head>");
							client.print("<h2>");
							sprintf(buf, "%02d:%02d:%02d รุ้ง:%7.5lf แวง:%7.5lf, SW:%d, SS:%d"
							, gps_hour, gps_minute, gps_second, gps_lat, gps_lng, g_sw1, g_alc);
							client.print(buf);
							client.print("</h2>");
							client.print("<h4>");
							//client.print(timeCnt);
							for(int i=0,c=timeCnt; c>=0 && i<(MAX_RECORD>10? 10:MAX_RECORD); i++, c--) {
								prec = &records[c%MAX_RECORD];
								sprintf(buf, "%d: %02d:%02d:%02d รุ้ง:%7.5lf แวง:%7.5lf, SW:%d, SS:%d"
								, c , prec->gps_hour, prec->gps_minute, prec->gps_second
								, prec->gps_lat, prec->gps_lng, prec->g_sw1, prec->g_alc);
								client.print(buf);
								client.println("<br>");
							}
							client.print("</h4>");
							client.println();
							client.println("</html>");
						} else if (header.indexOf("GET /data") >= 0) {
							client.println("HTTP/1.1 200 OK");
							client.println("Content-type:text/plain");
							client.println("Connection: close");
							client.println();
							for(int i=0,c=timeCnt; c>=0 && i<MAX_RECORD; i++, c--) {
								prec = &records[c%MAX_RECORD];
								sprintf(buf, "%d: %02d:%02d:%02d %7.5lf,%7.5lf H:%d SW:%d, SS:%d J:%d SW:%d"
								, c , prec->gps_hour, prec->gps_minute, prec->gps_second
								, prec->gps_lat, prec->gps_lng, prec->g_on, prec->g_sw1
								, prec->g_alc, prec->j_on, prec->j_sw1);
								client.println(buf);
							}
						} else {
							client.println("HTTP/1.1 200 OK");
							client.println("Content-type:text/html");
							client.println("Connection: close");
							client.println();
							client.println("<!DOCTYPE html><html lang=\"th\">");
							client.println("<html>");
							client.println("<H1>BIKE_WIFI</H1>");
							client.println("</html>");
						}
						break;
					} else { // if you got a newline, then clear currentLine
						currentLine = "";
					}
				} else if (c != '\r') {
					currentLine += c;
				}
			}
		}
		header = "";
		client.stop();
		Serial.println("Client disconnected.");
		//Serial.println("");
	}

	delay(250);
} // End of loop

