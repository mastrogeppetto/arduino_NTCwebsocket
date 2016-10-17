// Copyright 2016 - Augusto Ciuffoletti
// Evaluates the performance of WebSockets on Arduino using
// the library 
// https://github.com/ejeklint/ArduinoWebsocketServer
// commit 6fba6d1 (15 Jan 2015)

// #include <SD.h> // not for Arduino Duemilanove
#include <WebSocket.h>

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 113, 177 };
// sensor Pin
int sensorPin = A0;
// measurement time
long int t0;
// Timer driven trigger for measurements
volatile boolean trigger = false;
// Connection established
boolean isConnected=0;
// Ping protocol variables
boolean ack = false; // ack waiting
boolean ackReceived = true; // ack received 
long int tAck; //Ack received timestamp

// File dataFile;

// Create a Websocket server
WebSocketServer wsServer;

// callback when data received
void onData(WebSocket &socket, char* dataString, byte frameLength) {  
  ack = true;
  tAck = millis();
}

void onDisconnect(WebSocket &socket) {
  isConnected = false; //triggers cycle break
  Serial.println("onDisconnect called");
}

void setup() {
  Serial.begin(57600);
/*    
  SD.begin();
  if (!SD.begin(4)) {
    Serial.println("SD unit initialization failed!");
    return;
  }
  Serial.println("SD unit initialization done.");
  dataFile = SD.open("data.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("*** Start new log");
    dataFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("SD write error!");
  }
*/
  // Initialize Ethernet  
  Ethernet.begin(mac, ip);
  // Initialize WebSocket
  // when a new frame is received
  wsServer.registerDataCallback(&onData);
  wsServer.registerDisconnectCallback(&onDisconnect);
  wsServer.begin();
  // Give time to stabilize 
  delay(100);
  // initialize timer1 to tick every second
  // Do not use Servo library with this code
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 62500;            // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
  // finished with timer1
  Serial.println("Setup complete");
}

// This is the timer interrupt handler
SIGNAL(TIMER1_COMPA_vect) 
{
    trigger=true;
}

int measure(char* buffer){
  int i=0;
  t0=millis(); //record when measurement done
  dtostrf(t0/1000.0,-1,3,buffer);
  while (buffer[i] != '\0' && i < 80 ) i++;
  buffer[i++]='\t';
  itoa(analogRead(A0),buffer+i,10);
  while (buffer[i] != '\0' && i < 80 ) i++;
  return i;
}

void loop() {
  long int t1;
  int msglen;
  char buffer[80];
  // Wait for a connection
  wsServer.listen();
  isConnected = true;
  // Give time to stabilize
  delay(100);
  // Loop until connection closed
  while (wsServer.connectionCount() > 0) {
//  while ( isConnected ) {
      // See if it's time to send data
      if ( trigger ) {
        // if no Ack of previous msg close connection
        if ( ackReceived=false ) {
          Serial.println("No ack, restarting");
          break;
        }
        msglen=measure(buffer);
        wsServer.send(buffer, msglen);
        t1=millis(); // record time when send returns
        Serial.print(buffer);
        Serial.print("\t");
        Serial.print(t1-t0); // approx measurement latency (local)
        // Prepare ping protocol
        ackReceived=false;
        trigger = false;
        delay(10);
      }
      // See if a frame arrived
      wsServer.listen();
      if ( ack ) {
        Serial.print("\t");
        Serial.println(tAck-t0);
        ackReceived=true;
        ack = false;
        delay(10);
      }
      // This is the grain of RTT
      delay(10);
  }
}
