#include <SPI.h>
#include <LoRa.h>

// define the pins used by the transceiver module
#define ss 5
#define rst 14
#define dio0 2

// define pins used for leds
#define SetLight 33  

// define messages codes
#define HANDSHAKE 0x7E
#define PING 0xD0
#define PONG 0xDF
#define LAUNCH 0x4C
#define CONFIRMED 0x3F
#define ACK 0x6B
#define ABORT 0xFF
#define RST 0xCF

// variables
#define BlinkTime 500

enum State
{
  initial,
  set,
  con
} state;

void setup()
{
  // initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(SetLight, OUTPUT);
  digitalWrite(SetLight, LOW);

  Serial.println("LoRa Receiver");

  // setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);

  // 433E6 for Asia
  // 866E6 for Europe
  // 915E6 for North America
  while (!LoRa.begin(866E6))
  {
    Serial.println(".");
    delay(500);
  }

  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  LoRa.setSyncWord(0xF1);
  Serial.println("LoRa Initializing OK!");
  state = initial;
}

// launch function, return false in case of errors
bool launch()
{
  return true;
}

void sendByte(byte data)
{
  Serial.print("Sending '");
  Serial.print(data, HEX);
  Serial.print("'\n");

  LoRa.beginPacket();
  LoRa.write(data);
  LoRa.endPacket();
}

void blink()
{
  digitalWrite(SetLight, HIGH);
  delay(BlinkTime);
  digitalWrite(SetLight, LOW);
}

void loop()
{
  byte data = 0;
  // try to parse packet
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

    if (data == RST)
      state = initial;
    else
    {
      switch (state)
      {
        // initial Handshake
      case initial:
        if (data == HANDSHAKE)
        {
          sendByte(HANDSHAKE);
          state = set;
        }
        else
        {
          Serial.println("Error: Unexpected data");
        }
        break;

        // standby state
      case set:
        if (data == PING)
        {
          blink();
          sendByte(PONG);
        }
        else if (data == LAUNCH)
        {
          sendByte(ACK);
          state = con;
        }
        else
        {
          Serial.println("Error: Unexpected data");
        }
        break;

        // launch confirmation state
      case con:
        if (data == CONFIRMED)
        {
          if (launch())
          {
            Serial.println("Launched");
            sendByte(ACK);
          }
          else
          {
            Serial.println("Error: Failed to launch");
            sendByte(ABORT);
          }
        }
        break;

      default:
        break;
      }
    }
  }
}
