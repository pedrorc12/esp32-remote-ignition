#include <SPI.h>
#include <LoRa.h>

// define the pins used by the transceiver module
#define ss 5
#define rst 14
#define dio0 2

// define messages codes
#define HANDSHAKE 0x7E
#define PING 0xD0
#define PONG 0xDF
#define LAUNCH 0x4C
#define CONFIRMED 0x3F
#define ACK 0x6B
#define ABORT 0xFF
#define RST 0xCF

enum State
{
  initial,
  set,
  con,
  error
} state;

void setup()
{
  // initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("LoRa Sender");

  // setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);

  // replace the LoRa.begin(---E-) argument with your location's frequency
  // 433E6 for Asia
  // 866E6 for Europe
  // 915E6 for North America
  while (!LoRa.begin(866E6))
  {
    Serial.println(".");
    delay(500);
  }
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
  state = initial;
}

void sendByte(byte data)
{
  Serial.print("Sending packet: ");
  Serial.println(data, HEX);

  // Send LoRa packet to receiver
  LoRa.beginPacket();
  LoRa.write(data);
  LoRa.endPacket();
}

byte waitResponse()
{
  byte data = 0;
  while (true)
  {
    int packetSize = LoRa.parsePacket();
    if (packetSize)
    {
      // received a packet
      Serial.print("Received packet '");

      // read packet
      while (LoRa.available())
      {
        data = LoRa.read();
        Serial.print(data, HEX);
      }

      // print RSSI of packet
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
      return data;
    }
  }
}

void reset()
{
  byte response = 0;
  Serial.println("Error: Unexpected response sending RST...");
  sendByte(RST);

  response = waitResponse();

  if (response == ACK)
  {
    Serial.println("ACK received, reseting...");
    state = initial;
  }
  else
  {
    Serial.println("Something went wrong");
    state = error;
  }
}

bool launchCommand()
{
  return false;
}

void loop()
{
  byte response = 0;

  switch (state)
  {
  case initial:
    sendByte(HANDSHAKE);
    response = waitResponse();

    if (response == HANDSHAKE)
      state = set;

    break;

  case set:

    if (launchCommand())
    {
      sendByte(LAUNCH);

      response = waitResponse();
      if (response == ACK)
        state = con;
      else
        reset();
    }
    else
    { 
    sendByte(PING);
    response = waitResponse();

    if (response != PONG)
      reset();
    }
   
    break;

  case con:
    sendByte(CONFIRMED);

    response = waitResponse();
      if (response == ACK)
        Serial.println("Launch confirmed");
      else
        reset();

  default:
    break;
  }
}
