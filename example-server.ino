
#include <Esp.h>

//For the First time UPLOAD BLINK AND TYPE AT+UART_DEF=9600,8,1,0,0

#define BOUND_SERIAL 9600
#define BOUND_ESP 9600
//set your SSID here
#define SID "YOUR_SSID"
//set your PASS here
#define PASS "YOUR_PASSWORD"
//set your server IP here
#define IP "EXPECTED_IP"
#define HTTP_PORT 80
//define your RX and TX pins
#define RX_PIN 2
#define TX_PIN 3




SoftwareSerial espSerial(RX_PIN,TX_PIN);

void runInLoop();
void connectAndStartServer();

//module esp initialization
Esp esp(Serial,espSerial,runInLoop);

int requestNumber;


void setup() {

  Serial.begin(BOUND_SERIAL);
  espSerial.begin(BOUND_ESP);


  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  connectAndStartServer();

  
}

void loop() {
 runInLoop();
 esp.runInMainLoop(handleRequest);
 digitalWrite(LED_BUILTIN,requestNumber % 5 == 0 ? HIGH :LOW);
 esp.autoReconnectIfLost(SID,PASS,IP,HTTP_PORT);
 delay(50);
}
void connectAndStartServer() {
  esp.connectAndStartServer(SID,PASS,IP,HTTP_PORT);
}
void runInLoop() {
	//here you can execute some code f.e. read from sensors ect
	//this code will be executed also during reading response from ESP
}
void handleRequest(char* requestBody) {
    Serial.println("----------- HANDLING REQUEST  ---------------");
    requestNumber++;
    String html = String(requestNumber);
    esp.writeHeadersToClient();
    esp.writeToClient(html);
    esp.closeClientConnection();

}

