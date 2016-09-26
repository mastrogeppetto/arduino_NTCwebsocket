// Copyright 2016 - Augusto Ciuffoletti
// Evaluates the performance of WebSockets on Arduino using
// the library 
// https://github.com/ejeklint/ArduinoWebsocketServer
// commit 6fba6d1 (15 Jan 2015)

// #include <SD.h>
#include <WebSocket.h>

byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 113, 177 };
volatile boolean trigger = false;
boolean ack = false;
long int tAck;

//File dataFile;

// Create a Websocket server
WebSocketServer wsServer;

// callback when data received
void onData(WebSocket &socket, char* dataString, byte frameLength) {  
  ack = true;
  tAck = millis();
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

  Ethernet.begin(mac, ip);

  // when a new frame is received
  wsServer.registerDataCallback(&onData);
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
  Serial.println("Setup finished");
}

// This is the timer interrupt handler
SIGNAL(TIMER1_COMPA_vect) 
{
    trigger=true;
}

void loop() {
  long int t0;
  long int t1;
  // Wait for a connection
  wsServer.listen();
  // Give time to stabilize
  delay(100);
  // Loop until connection closed
  while (wsServer.connectionCount() > 0) {
      // See if it's time to send data
      if ( trigger ) {
        t0=millis();
        t1=0;
        String msg=String(t0/1000.0,3);
        byte buffer[msg.length()];
        msg.getBytes(buffer, msg.length()+1);
        wsServer.send(buffer, msg.length()+1);
        t1=millis();
        Serial.println(msg);
        Serial.println(t1-t0);
        trigger = false;
      }
      // See if a frame arrived
      wsServer.listen();
      if ( ack ) {
        Serial.print("Ack received at ");
        Serial.println(tAck-t0);
        ack = false;
      }
      // This is the grain of RTT
      delay(10);
  }
}
