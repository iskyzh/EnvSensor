#include <DHT.h>

#define DHTTYPE DHT22
#define DHTPIN 7

#define DATA_CMD_TEMPERATURE 97
#define DATA_CMD_PM 65
#define DATA_CMD_DEBUG 66
#define DATA_CMD_FAILED 98

#define PACK_HEADER_LENGTH 2
#define PACK_DHT_LENGTH 20
#define PACK_RAW_LENGTH 20

typedef unsigned char CMD;
typedef unsigned char SIZE;

DHT dht(DHTPIN, DHTTYPE);

struct Header {
  CMD cmd;
  SIZE sz;
} Header;

union HeaderPack {
  unsigned char pack[PACK_HEADER_LENGTH];
  struct Header data;
} HeaderPack;

struct DHTData {
  float h, t, f, hif, hic;
} DHTData;

union DHTPack {
  unsigned char pack[PACK_DHT_LENGTH];
  struct DHTData data;
} DHTPack;

struct RawPack {
  unsigned char pack[PACK_RAW_LENGTH];
} RawPack;

void setup()
{
  Serial.begin(115200);
  dht.begin();
}

int getDHTData(struct DHTData *data) {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    return 0;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  data->h = h; data->t = t; data->f = f; data->hif = hif; data->hic = hic;
  return 1;
}

int readCommand(HardwareSerial *serial) {
  if (serial->available() > 0) {
    return serial->read();
  }
  return 0;
}

void getHeader(struct Header* header, CMD cmd, SIZE sz) {
  header->cmd = cmd;
  header->sz = sz;
}

int writeCommand(HardwareSerial *serial, CMD cmd, SIZE sz, char* data) {
  union HeaderPack headerPack;
  getHeader(&headerPack.data, cmd, sz);
  
  serial->write(headerPack.pack, PACK_HEADER_LENGTH);
  serial->write(data, sz);
}

void loop() {
  union DHTPack dhtPack;
  struct RawPack rawPack;
  
  CMD cmd = 0;
  
  while(1) {
    while(!(cmd = readCommand(&Serial)));
    
    if(cmd == DATA_CMD_TEMPERATURE) {
      if(getDHTData(&dhtPack.data)) {
        writeCommand(&Serial, cmd, PACK_DHT_LENGTH, ((char*)dhtPack.pack));
      } else {
        writeCommand(&Serial, DATA_CMD_FAILED, PACK_RAW_LENGTH, ((char*)rawPack.pack));
      }
    } else if(cmd == DATA_CMD_DEBUG) {
      writeCommand(&Serial, DATA_CMD_DEBUG, PACK_RAW_LENGTH, ((char*)rawPack.pack));
    }
  }
}