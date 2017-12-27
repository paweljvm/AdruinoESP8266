#ifndef Esp_h
#define Esp_h
#include "Arduino.h"
#include "SoftwareSerial.h"


#define C_INFO "AT"
#define C_MODE "AT+CWMODE"
#define C_MULTIPLE_CONNECTIONS "AT+CIPMUX"
#define C_SERVER "AT+CIPSERVER"
#define C_GET_IP "AT+CIFSR"
#define C_SET_IP "AT+CIPSTA_CUR"
#define C_SEND_DATA "AT+CIPSEND" 
#define C_CLOSE "AT+CIPCLOSE"
#define C_CIPSTATUS "AT+CIPSTATUS"
#define C_CONNECT  "AT+CWJAP"
#define C_RESET "AT+RST"

#define R_OK "OK"
#define R_ERROR "ERROR"
#define BUFFER 500

#define M_STATION 1
#define M_AP 2 
#define M_AP_STATION 3

#define EQUAL "="
#define QUESTION_MARK "?"
#define EMPTY ""
#define COMA ","
#define SPACE " "
#define EQUAL "="
#define AMP "&"
#define COLON ":"
#define NEW_LINE "\n"
#define END_LINE "\r\n"
#define SLASH "/"
#define D_SHORT 30
#define D_LONG 10000
#define D_MEDIUM 2000
#define REQUEST_PREFIX "+IPD"
#define IP_RESPONSE_PREFIX "+CIFSR:STAIP"
#define CONNECT_RESPONSE_PREFIX "+CWJAP:"
#define ENABLE 1
#define DISABLE 0
#define MAX_HEADERS 10
#define MAX_PARAMS 10
#define HTTP_OK_STATUS "HTTP/1.1 200 OK"
#define DEFAULT_SERVER "ESP8266"
#define H_SERVER "Server:"
#define CHECK_CONNECTION_DELAY 20000
enum CommandType {
  CHECK,SET,INFO
};
enum Mode {
 STATION,AP,STATION_AP
};

class KeyValue {
	public:
	KeyValue();
	KeyValue(String key,String value);
	String _key;
	String _value;
};
class Request {
	public:
	Request(String uri,KeyValue* headers,KeyValue* params);
	~Request();
	String _uri;
	KeyValue* _headers;
	KeyValue* _params;
};

class Esp {
	public:
	Esp(HardwareSerial &print,SoftwareSerial &esp,void (*loopFun)(void),String serverName=DEFAULT_SERVER);
	boolean reset();
	boolean check();
	boolean setMode(Mode mode);
	boolean connect(String sid,String password);
	boolean setIp(String ip);
	String getIp();
	boolean startServer(int port);
	boolean isAvailable();
	boolean writeToClient(String data);
	boolean writeHeadersToClient();
	char* readFromEspOnSerial(int delayValue);
	boolean closeClientConnection();
	boolean isRequest(char* value);
	boolean connectAndStartServer(String sid,String pass,String ip,int httpPort);
	void runInMainLoop(void (*requestHandler)(char*));
	void autoReconnectIfLost(String sid,String pass,String ip,int httpPort);
	boolean isConnected();
	Request parseRequestBody(char* requestBody);
	String Esp::getUri(char* requestBody);
	private:
	HardwareSerial *printer;
	SoftwareSerial *_esp;
	String _serverName;
	unsigned long checkTime;
	void (*_loopFun) (void);
	char* writeOnEsp(String command,int delayValue);
	char* writeOnEsp(String command,CommandType type,int delayValue);
	char* writeOnEsp(String command,String args,CommandType type,int delayValue);
	String getCommand(String command,String args,CommandType type);
	String readFromSerial();
	boolean nullOrEmpty(String string);
	char* writeOnEspAndGetResponse(String str,int delayValue);
	boolean checkIfResponseIsOk();
	boolean checkIfContains(char* value,char* test);
	void clearBuffer(char* buffer);
	String wrapInQuotes(String value);
	KeyValue* parseParams(String uri);
	KeyValue* parseHeaders(String body);
	KeyValue* parseValues(int size,String text, String keyValueDivider,String nextValueDivider);
	
};

#endif
