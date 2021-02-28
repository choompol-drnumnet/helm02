#include <Arduino.h>
#include <WiFi.h>
extern "C" {
#include <esp_wifi.h>
}

int status = WL_IDLE_STATUS;
WiFiClient client;

void WiFiEventHandler(WiFiEvent_t event) {
	Serial.printf("Got Event: %d\n", event);
}

void setup() {
	Serial.begin(115200);
	Serial.println("Starting BLE work!");
	WiFi.onEvent(WiFiEventHandler);

	int i=0;
	for(;;) {
		i++;
		Serial.println("==================================");
		Serial.println(i);

		Serial.println("try connect WIFI");
	 	delay(500);
		WiFi.begin("BIKE_WIFI1", "0123456789");
		for (int c=0; c<30 && (status=WiFi.status()) != WL_CONNECTED; c++) {
			delay(500);
		}

		if(status == WL_CONNECTED) {
			Serial.println("WIFI connected");
		} else {
			Serial.println("Can't connect WIFI");
			continue;
		}
		Serial.println(WiFi.localIP());
		Serial.println();

		Serial.println("\nStarting connection to server...");
		if (client.connect("192.168.4.1", 80)) {
			Serial.println("connected to server");
			client.println("GET /data HTTP/1.1");
			client.println("Host: 192.168.4.1");
			client.println("Connection: close");
			client.println();
		}

		delay(500);
		for(int c=0; c<10; c++) {
			while (client.available()) {
				char c = client.read();
				Serial.write(c);
			}
			delay(500);
		}
		client.stop();
		Serial.println("...");
		delay(500);

		Serial.println("try disconnect WIFI");
		delay(500);
		WiFi.disconnect();
		//esp_wifi_disconnect();
		//esp_wifi_stop();
		//esp_wifi_deinit();
		for(int c=0; c<30 && (status=WiFi.status()) == WL_CONNECTED; c++) {
			status = WiFi.status();
			delay(500);
		}
		if(status == WL_CONNECTED) {
			Serial.println("Disconnect failed");
			continue;
		} else {
			Serial.println("Disconnect success");
		}
		Serial.println();
		delay(1000);

		Serial.println("try connect WIFI2");
	 	delay(1000);
		WiFi.begin("CHOOMPOL_WIFI3", "choompol");
		//WiFi.begin("HI_BMFWIFI_2.4G", "0819110933");
		for (int c=0; c<30 && (status=WiFi.status()) != WL_CONNECTED; c++) {
			delay(500);
		}

		if(status == WL_CONNECTED) {
			Serial.println("WIFI2 connected");
		} else {
			Serial.println("Can't connect WIFI2");
			continue;
		}

		Serial.println(WiFi.localIP());
		Serial.println();

		Serial.println("==== try disconnect WIFI2");
		delay(500);
		WiFi.disconnect();
		for(int c=0; c<30 && (status=WiFi.status()) == WL_CONNECTED; c++) {
			status = WiFi.status();
			delay(500);
		}

		if(status == WL_CONNECTED) {
			Serial.println("Disconnect WIFI2 failed");
			continue;
		} else {
			Serial.println("Disconnect WIFI2 success");
		}
		Serial.println();
		delay(2000);

	}
} 

void loop() {
}

