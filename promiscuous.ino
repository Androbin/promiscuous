#include <ESP8266WiFi.h>

static const uint8_t CRC_TABLE[] PROGMEM = {
  0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
  0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
  0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
  0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
  0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
  0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
  0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
  0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
  0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
  0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
  0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
  0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
  0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
  0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
  0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
  0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
};

const uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct Packet {
  uint8_t x[16];
  uint8_t target[6];
  uint8_t y[6];
  uint8_t originA[6];
  uint8_t z[16];
  uint16_t length;
  uint16_t seq;
  uint8_t originB[6];
};

struct DataBuffer {
  uint8_t nonzero;
  char ssid[32];
  char pass[64];
};

uint8_t origin[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

uint8_t guide;
boolean guided;

uint16_t lastLength;
uint16_t refLength;

uint16_t lengthX;
uint16_t lengthY;

DataBuffer dataBuffer;
uint8_t nonzero;

uint8 channel = 1;
unsigned int nothing_new = 0;

void promiscuous_cb(uint8_t *buffer, uint16_t bufferLength);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  wifi_set_opmode(STATION_MODE);
  wifi_set_sleep_type(NONE_SLEEP_T);
  wifi_set_channel(channel);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(promiscuous_cb);
  wifi_promiscuous_enable(1);

  reset(true);
}

void loop() {
  nothing_new++;

  if (nothing_new > 200) {
    nothing_new = 0;
    channel = channel % 13 + 1;
    // wifi_set_channel(channel);
  }

  delay(1);
}

boolean cmpMac(const uint8_t a[6], const uint8_t b[6]) {
  for (size_t i = 0; i < 6; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }

  return true;
}

boolean copyMac(uint8_t d[6], const uint8_t s[6]) {
  for (size_t i = 0; i < 6; i++) {
    d[i] = s[i];
  }
}

void reset(boolean all) {
  lengthX = 0;
  lengthY = 0;

  uint8_t* dataBufferRaw = reinterpret_cast<uint8_t*>(&dataBuffer);

  for (size_t i = 0; i < sizeof(DataBuffer); i++) {
    dataBufferRaw[i] = 0;
  }

  nonzero = 0;

  if (!all) {
    return;
  }

  guide = 0;
  guided = false;

  lastLength = 0;
  refLength = 0;
}

void ICACHE_RAM_ATTR handleUdpData(uint16_t x, uint16_t y, uint16_t z) {
  uint8_t* dataBufferRaw = reinterpret_cast<uint8_t*>(&dataBuffer);

  const uint8_t crc = (x & 0xf0) | ((z & 0xf0) >> 4);
  const uint8_t seq = y & 0xff;
  const uint8_t data = ((x & 0x0f) << 4) | (z & 0x0f);

  const uint8_t xcrc = pgm_read_byte(&CRC_TABLE[pgm_read_byte(&CRC_TABLE[data]) ^ seq]);

  if (crc != xcrc) {
    return;
  }

  if (seq >= sizeof(DataBuffer) || data == 0) {
    return;
  }

  if (dataBufferRaw[seq] != 0) {
    return;
  }

  dataBufferRaw[seq] = data;
  nonzero++;

  if (nonzero != dataBuffer.nonzero) {
    return;
  }

  Serial.print(F("ssid: "));
  Serial.println(dataBuffer.ssid);
  Serial.print(F("pass: "));
  Serial.println(dataBuffer.pass);
}

void ICACHE_RAM_ATTR promiscuous_cb(uint8_t *buffer, uint16_t bufferLength) {
  if (bufferLength != sizeof(Packet)) {
    return;
  }

  Packet *packet = reinterpret_cast<struct Packet*>(buffer);

  if (!cmpMac(packet->target, broadcast)) {
    return;
  }

  if (!cmpMac(packet->originA, packet->originB)) {
    return;
  }

  if (!cmpMac(packet->originA, origin)) {
    if (packet->length < 0x200) {
      return;
    }

    copyMac(origin, packet->originA);
    reset(true);
  }

  nothing_new = 0;

  if (packet->length >= 0x200 && lastLength - packet->length == 1) {
    guide++;

    if (guide == 3) {
      refLength = packet->length - 0x200;
      guided = true;

      if (nonzero) {
        reset(false);
      }
    }
  } else {
    guide = 0;
  }

  lastLength = packet->length;

  if (!guided || (nonzero > 0 && nonzero == dataBuffer.nonzero)) {
    return;
  }

  const uint16_t realLength = packet->length - refLength;
  const uint16_t lengthZ = realLength - 40;

  if (lengthZ >= 0x200) {
    return;
  }

  if ((lengthY & 0x100) && !(lengthZ & 0x100)) {
    handleUdpData(lengthX, lengthY, lengthZ);
  }

  lengthX = lengthY;
  lengthY = lengthZ;
}
