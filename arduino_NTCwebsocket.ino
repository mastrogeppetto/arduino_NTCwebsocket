// Copyright 2016 - Augusto Ciuffoletti
// Evaluates the performance of WebSockets on Arduino using
// the library 
// https://github.com/ejeklint/ArduinoWebsocketServer
// commit 6fba6d1 (15 Jan 2015)

// #include <SD.h> // not for Arduino Duemilanove
#include <Ethernet.h>
#include <WebSocket.h>

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 113, 177 };
// sensor Pin
int sensorPin = A0;
// measurement time
long int t0;
// Timer driven trigger for measurements
volatile boolean trigger = false;
// Ping protocol variables
boolean ackReceived = true; // ack received 
long int tAck; //Ack received timestamp
char buffer[80];

// File dataFile;

// Create a Websocket server
WebSocketServer wsServer;

// callback when data received
void onData(WebSocket &socket, char* dataString, byte frameLength) {  
  // Only ack messages are received...
  ackReceived=true;
  Serial.print("\t");
  Serial.println(millis()-t0);
}

void onDisconnect(WebSocket &socket) {
  Serial.println("onDisconnect called");
}

void onConnect(WebSocket &socket) {
  Serial.println("onConnect called");
}

void setup() {
  Serial.begin(57600);
  Serial.println("Hallo!!");
  // Initialize Ethernet  
  Ethernet.begin(mac, ip);
  // Initialize WebSocket
  // when a new frame is received
  wsServer.registerDataCallback(&onData);
  wsServer.registerDisconnectCallback(&onDisconnect);
  wsServer.registerConnectCallback(&onConnect);
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

int measure(){
  int i=0;
  char a[16];
  t0=millis(); //record when measurement done
  dtostrf(t0/1000.0,-1,3,a);
  strcpy(buffer,a);
  strcat(buffer,"\t");
  itoa(analogRead(A0),a,10);
  strcat(buffer,a);
  return strlen(buffer);
}

void loop() {
  long int t1;
  int msglen;
  // Wait for a connection
  // See if a frame arrived
  wsServer.listen();
  // Give time to stabilize
  delay(10); // !!!! Granularity of RTT
  // In case there are connections
  if (wsServer.connectionCount() > 0) {
      // See if it's time to send data
      if ( trigger ) {
        // if no Ack of previous msg close connection
        if ( ackReceived==false ) {
          Serial.println("\tNo ack");
        }
        msglen=measure();
        wsServer.send(buffer, msglen);
        t1=millis(); // record time when send returns
        Serial.print(buffer);
        Serial.print("\t");
        Serial.print(t1-t0); // approx measurement latency (local)
        // Prepare ping protocol
        ackReceived=false;
        trigger = false;
      }
  }
}
