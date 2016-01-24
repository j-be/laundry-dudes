#include <Process.h>
#include <SoftwareSerial.h>
#include <Console.h>

#define DATA_URI "http://192.168.1.147:5000/laundrydude/api/data"
#define BLOCKER_STATE_URI "http://192.168.1.147:5000/laundrydude/api/blocker"

const byte numChars = 32;
char receivedChars[numChars];		// an array to store the received data
bool newData = false;
char block = false;

SoftwareSerial xbeeSerial(8, 9); // RX, TX
Process curl;

int requested[] = {0, 0, 0, 0, 0};
String requests = "htlar";

int ndx = 0;

void setup() {
	Bridge.begin();

	Console.begin();
	xbeeSerial.begin(9600);

	// Setup lock
	pinMode(A0, OUTPUT);
	setLockState(false);
}

void setLockState(bool locked) {
	digitalWrite(A0, locked);
}

void loop() {
	for (int i = 0; i < requests.length(); i++) {
		request(requests.charAt(i));
		while(processResponse());
	}

	for (int i = 0; i < 10000; i++)
		processResponse();
}

void set(char type, char value) {
	xbeeSerial.print(type);
	xbeeSerial.println(value);
}

bool processResponse() {
	char type;
	int indexOfType;

	while (recvWithEndMarker()) {
		type = receivedChars[0];
		indexOfType = requests.indexOf(type);
		if (indexOfType < 0)
			return false;
		sendData();
		requested[indexOfType] = 0;
		return true;
	}
}

void request(char type) {
	requested[requests.indexOf(type)] %= 50;
	requested[requests.indexOf(type)]++;

	if (requested[requests.indexOf(type)] != 1) {
		return;
	}

	Console.print("Request: ");Console.println(type);
	xbeeSerial.println(type);
}

void sendData() {
	Console.print("Sending: ");
	Console.println(receivedChars);
	postData(receivedChars);
}

bool recvWithEndMarker() {
	char endMarker = '\n';
	char rc;

	while (xbeeSerial.available()) {
		rc = xbeeSerial.read();
		Console.print(rc);

		if (rc != endMarker) {
			receivedChars[ndx] = rc;
			ndx++;
			if (ndx >= numChars)
				ndx = numChars - 1;
		} else {
			receivedChars[ndx] = '\0'; // terminate the string
			ndx = 0;
			return true;
		}
	}

	return false;
}

void postData(const char* data) {
	if (curl.running())
		curl.close();

	curl.begin("curl");
	curl.addParameter("-X");
	curl.addParameter("POST");
	curl.addParameter(DATA_URI);
	curl.addParameter("-d");
	curl.addParameter(data);
	curl.runAsynchronously();
}

void getData() {
	Process p;
	p.begin("curl");
	p.addParameter(BLOCKER_STATE_URI);
	p.run();

	while (p.available()>0) {
		char c = p.read();
		switch(c) {
			case 'b':
				if (p.read() == '=')
					block = p.read();
				break;
		}
	}
}
