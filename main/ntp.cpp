#include <Arduino.h>
#include <WiFi.h>
#include "ntp.h"

bool ntpUpdate(const char *server, int8_t tz, uint32_t timeout, uint8_t repeat) {
  constexpr uint16_t LOCAL_PORT = 55123;

  WiFiUDP udp;

  if (udp.begin(LOCAL_PORT)) {
    do {
      uint8_t buffer[48];

      memset(buffer, 0, sizeof(buffer));
      // Initialize values needed to form NTP request
      buffer[0] = 0B11100011; // LI, Version, Mode
      buffer[1] = 0; // Stratum, or type of clock
      buffer[2] = 6; // Polling Interval
      buffer[3] = 0xEC; // Peer Clock Precision
      // 8 bytes of zero for Root Delay & Root Dispersion
      buffer[12] = 49;
      buffer[13] = 0x4E;
      buffer[14] = 49;
      buffer[15] = 52;
      // all NTP fields have been given values, now
      // you can send a packet requesting a timestamp
      if (udp.beginPacket(server, 123) && (udp.write(buffer, sizeof(buffer)) == sizeof(buffer)) && udp.endPacket()) {
        uint32_t time = millis();
        int cb;

        while ((! (cb = udp.parsePacket())) && (millis() - time < timeout)) {
          delay(1);
        }
        if (cb) {
          // We've received a packet, read the data from it
          if (udp.read(buffer, sizeof(buffer)) == sizeof(buffer)) { // read the packet into the buffer
            timeval tv;

            // the timestamp starts at byte 40 of the received packet and is four bytes,
            // or two words, long. First, esxtract the two words:
            time = (((uint32_t)buffer[40] << 24) | ((uint32_t)buffer[41] << 16) | ((uint32_t)buffer[42] << 8) | buffer[43]) - 2208988800UL;
            time += tz * 3600;
            tv.tv_sec = time;
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            return true;
          }
        }
      }
      if (repeat)
        delay(timeout / 2);
    } while (repeat--);
  }
  return false;
}
