#include "Esp.h"

KeyValue::KeyValue() {

}
KeyValue::KeyValue(String key, String value) {
	_key = key;
	_value = value;
}
Request::Request(String uri, KeyValue* headers, KeyValue* params) {
	_uri = uri;
	_headers = headers;
	_params = params;
}
Request::~Request() {
	delete _headers;
	delete _params;
}
Esp::Esp(HardwareSerial &print, SoftwareSerial &esp, void (*loopFun)(void),
		String serverName = DEFAULT_SERVER) {
	printer = &print;
	_esp = &esp;
	_loopFun = loopFun;
	_serverName = serverName;

}
void Esp::runInMainLoop(void (*requestHandler)(char*, int)) {
	if (isAvailable()) {
		processingRequest = true;
		int connectionId = _esp->read() - 48;
		if (_esp->find(REQUEST_PREFIX)) {
			String request = "";
			char sign;
			int connectionId = -1;
			while ((sign = _esp->read()) != '\n') {
				request += sign;
				if(connectionId == -1 && sign >='0' && sign<='4') {
					connectionId = sign-48;
				}
			}
			char reqArray[request.length()];
			request.toCharArray(reqArray, request.length());
			printer->println("Req = " + request);
			requestHandler(reqArray, connectionId);
			processingRequest = false;
		}

	}

}
void Esp::autoReconnectIfLost(String sid, String pass, String ip,
		int httpPort) {
	if (!processingRequest && !isAvailable()
			&& (checkTime + CHECK_CONNECTION_DELAY) < millis()) {
		if (!isConnected()) {
			printer->println("Recon");
			delay (D_MEDIUM);
			connectAndStartServer(sid, pass, ip, httpPort);
		}
		checkTime = millis();
	}
}

boolean Esp::connectAndStartServer(String sid, String pass, String ip,
		int httpPort) {
	printer->println("--ST--");
	if (check()) {
		printer->println("ESP OK");
	} else {
		printer->print("ESP ERROR");
		return false;
	}
	printer->println("--RST--");
	reset();

	printer->println("-- STA MODE --");
	setMode (STATION);

	printer->println("--CONN--");
	connect(sid, pass);
	setIp(ip);
	String responseIp = getIp();
	printer->println("IP = " + responseIp);
	printer->println("--ST SVR--");
	checkTime = millis();
	return startServer(httpPort);
}
boolean Esp::isAvailable() {
	return _esp->available();
}
boolean Esp::reset() {
	return checkIfContains(writeOnEsp(C_RESET, D_SHORT), R_OK);
}
boolean Esp::check() {
	return checkIfContains(writeOnEsp(C_INFO, D_SHORT), R_OK);
}
boolean Esp::setMode(Mode mode) {
	return checkIfContains(writeOnEsp(C_MODE, String(mode + 1), SET, D_MEDIUM),
			R_OK);
}
boolean Esp::connect(String sid, String pass) {
	return checkIfContains(
			writeOnEsp(C_CONNECT,
					wrapInQuotes(String(sid)) + ","
							+ wrapInQuotes(String(pass)), SET, D_LONG), R_OK);
}
boolean Esp::makeRequest(String method, String url, String host, int port) {
	if (checkIfContains(
			writeOnEsp(C_CONNECTION_START,
					wrapInQuotes(String(TCP)) + "," + wrapInQuotes(host) + ","
							+ String(port), SET, D_SHORT), R_OK)) {
		String request = String(LT) + " " + method + " " + url + " "
				+ String(HTTP_1_1) + String(NEW_LINE) + String(H_HOST) + " "
				+ host + " " + String(NEW_LINE);
		writeOnEsp(C_SEND_DATA, String(request.length()), SET, D_SHORT);
		return checkIfContains(writeOnEspAndGetResponse(request, D_SHORT), R_OK);
	}

}

boolean Esp::setIp(String ip) {
	return checkIfContains(
			writeOnEsp(C_SET_IP, wrapInQuotes(ip), SET, D_MEDIUM), R_OK);
}
boolean Esp::setDomain(String domain) {
	return checkIfContains(
			writeOnEsp(C_DOMAIN, wrapInQuotes(domain), SET, D_MEDIUM), R_OK);
}
boolean Esp::isConnected() {
	char* responseBuffer = writeOnEsp(C_CONNECT, CHECK, D_SHORT);
	return (checkIfContains(responseBuffer,
			CONNECT_RESPONSE_PREFIX) ||
			checkIfContains(responseBuffer,R_BUSY))
			&& !checkIfContains(writeOnEsp(C_CIPSTATUS, INFO, D_SHORT), R_ERROR);
}

String Esp::getIp() {
	String result = String(writeOnEsp(C_GET_IP, D_MEDIUM));
	int from = result.indexOf(IP_RESPONSE_PREFIX);
	int start = result.indexOf(COMA, from);
	int end = result.indexOf(NEW_LINE, start);
	if (from != -1 && start != -1) {
		return result.substring(start + 1, end);
	}
	return result;
}
boolean Esp::startServer(int port) {
	writeOnEsp(C_MULTIPLE_CONNECTIONS, String(ENABLE), SET, D_SHORT);
	return checkIfContains(
			writeOnEsp(C_SERVER, String(ENABLE) + "," + String(port), SET,
					D_MEDIUM), R_OK);
}

boolean Esp::writeToClient(String data, int connectionId) {
	writeOnEsp(C_SEND_DATA, String(connectionId) + "," + String(data.length()),
			SET, D_SHORT);
	return checkIfContains(writeOnEspAndGetResponse(data, D_SHORT), R_OK);
}
boolean Esp::writeHeadersToClient(int connectionId) {
	writeToClient(String(HTTP_OK_STATUS) + NEW_LINE, connectionId);
	writeToClient(String(H_SERVER) + String(_serverName) + NEW_LINE,
			connectionId);
	writeToClient(NEW_LINE, connectionId);
}
boolean Esp::closeClientConnection(int connectionId) {
	return checkIfContains(
			writeOnEsp(C_CLOSE, String(connectionId), SET, D_SHORT), R_OK);
}
char* Esp::writeOnEsp(String command, int delayValue) {
	return writeOnEsp(command, "", INFO, delayValue);
}
char* Esp::writeOnEsp(String command, CommandType type, int delayValue) {
	return writeOnEsp(command, "", type, delayValue);
}
char* Esp::writeOnEsp(String command, String args, CommandType type,
		int delayValue) {
	String fullCommand = getCommand(command, args, type);
	_esp->print(fullCommand);
	return readFromEspOnSerial(delayValue);
}
char* Esp::writeOnEspAndGetResponse(String str, int delayValue) {
	_esp->print(str);
	return readFromEspOnSerial(delayValue);
}
boolean Esp::isRequest(char* value) {
	return checkIfContains(value, REQUEST_PREFIX);
}

Request Esp::parseRequestBody(char* requestBody) {
	String body = String(requestBody);
	int uriStartIndex = body.indexOf(SLASH);
	int uriEndIndex = body.indexOf(SPACE, uriStartIndex);
	String uri = body.substring(uriStartIndex, uriEndIndex);
	int questionMarkIndex = uri.indexOf(QUESTION_MARK);
	KeyValue* headers = parseHeaders(
			body.substring(body.indexOf(NEW_LINE) + 1, body.length()));
	KeyValue* params = parseParams(uri);
	uri = questionMarkIndex == -1 ? uri : uri.substring(0, questionMarkIndex);
	return Request(uri, NULL, NULL);
}
String Esp::getUri(char* requestBody) {
	char buffer[50];
	int length = strlen(requestBody);
	int index = 0;
	boolean found;
	for (int i = 0; i < length; i++) {
		if (requestBody[i] == '/') {
			found = true;
		} else if (requestBody[i] == ' ') {
			found = false;
		}
		if (found && index < 50) {
			buffer[index++] = requestBody[i];
		}
		if (!found && index > 0) {
			break;
		}
	}
	buffer[index] = 0;
	return String(buffer);
}

KeyValue* Esp::parseParams(String uri) {
	int questionMarkIndex = uri.indexOf(QUESTION_MARK);
	if (questionMarkIndex != -1) {
		String params = uri.substring(questionMarkIndex + 1, SPACE);
		return parseValues(MAX_PARAMS, params, EQUAL, AMP);
	}
	return NULL;
}
KeyValue* Esp::parseHeaders(String headers) {
	return parseValues(MAX_HEADERS, headers, COLON, NEW_LINE);
}
KeyValue* Esp::parseValues(int size, String text, String keyValueDivider,
		String nextValueDivider) {
	KeyValue result[size];
	int index = 0;
	for (int i = 0, n = text.length(); i < n;) {
		if (index == size)
			break;
		int startIndexOf = text.indexOf(keyValueDivider, i);
		int endIndexOf = text.indexOf(nextValueDivider, i);
		if (endIndexOf == -1)
			endIndexOf = n;
		String name = text.substring(i, startIndexOf);
		String value = text.substring(startIndexOf, endIndexOf);
		i = endIndexOf;
		result[index]._key = name;
		result[index++]._value = value;
	}

	return result;
}

boolean Esp::checkIfContains(char* value, char* test) {
	int l = strlen(test);
	for (int i = 0, n = strlen(value); i < n; i++) {
		int matches = 0;
		for (int j = 0; j < l; j++) {
			if (value[i + j] == test[j])
				matches++;
			else
				break;
		}
		if (matches == l) {
			return true;
		}
	}
	return false;

}
char* Esp::readFromEspOnSerial(int delayValue) {
	int tempPos = 0;
	char reply[BUFFER];
	clearBuffer(reply);
	long int time = millis();
	while ((time + delayValue) > millis()) {
		_loopFun();
		while (_esp->available()) {
			char c = _esp->read();
			printer->print(c);
			if (tempPos < BUFFER) {
				reply[tempPos++] = c;
			}
			if(processingRequest && checkIfContains(reply,END_OF_COMMAND_REQUEST)) {
				reply[++tempPos]=0;
				return reply;
			}
			
			_loopFun();
		}
		reply[tempPos] = 0;
	}
	return reply;
}
String Esp::getCommand(String command, String args, CommandType type) {
	String between = "";
	switch (type) {
	case CHECK:
		between = QUESTION_MARK;
		break;
	case SET:
		between = EQUAL;
		break;
	default:
		break;
	}
	return command + between + (!nullOrEmpty(args) ? args : "") + END_LINE;

}
String Esp::readFromSerial() {
	String result = "";
	while (printer->available()) {
		result = printer->readString();
	}
	return result;
}
boolean Esp::nullOrEmpty(String string) {
	return string == NULL || string.length() == 0;
}
boolean Esp::checkIfResponseIsOk() {
	return checkIfContains("OK / /ALLL", R_OK);
}
void Esp::clearBuffer(char* buffer) {
	for (int i = 0; i < BUFFER; i++) {
		buffer[i] = 0;
	}
}
String Esp::wrapInQuotes(String value) {
	return "\"" + value + "\"";
}

