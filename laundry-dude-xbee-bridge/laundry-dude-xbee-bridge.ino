#include <Process.h>
#include <SoftwareSerial.h>
#include <Console.h>

#define SERVER_URI "http://192.168.1.5:5000/laundrydude/api/data"

const byte numChars = 32;
char receivedChars[numChars];		// an array to store the received data
bool newData = false;

SoftwareSerial xbeeSerial(8, 9); // RX, TX
Process curl;

void setup() {
	Bridge.begin();

	Serial.begin(9600);
	xbeeSerial.begin(9600);
}

void loop() {
	// put your main code here, to run repeatedly:
	recvWithEndMarker();
	showNewData();
}

void showNewData() {
	if (newData == true) {
		Serial.print("This just in ... ");
		Serial.println(receivedChars);
		postData(receivedChars);
		newData = false;
	}
}

void recvWithEndMarker() {
	static byte ndx = 0;
	char endMarker = '\n';
	char rc;

	while (xbeeSerial.available() > 0 && newData == false) {
		rc = xbeeSerial.read();

		if (rc != endMarker) {
			receivedChars[ndx] = rc;
			ndx++;
			if (ndx >= numChars) {
				ndx = numChars - 1;
			}
		}
		else {
			receivedChars[ndx] = '\0'; // terminate the string
			ndx = 0;
			newData = true;
		}
	}
}

void postData(const char* data) {
	if (curl.running())
		curl.close();

	curl.begin("curl");
	curl.addParameter("-X");
	curl.addParameter("POST");
	curl.addParameter(SERVER_URI);
	curl.addParameter("-d");
	curl.addParameter(data);
	curl.runAsynchronously();
}

