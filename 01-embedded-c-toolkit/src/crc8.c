#include<stdint.h>
#include<stddef.h>
#include<assert.h>
#include "crc8.h"

uint8_t crc8(const uint8_t *data, size_t length){
   assert(length == 0 || data != NULL);

   uint8_t crc = 0x00;
   for (size_t len = 0; len < length; len++){
      crc ^= data[len];

      for (int i = 0; i < 8; i++){
         bool record = false;
         if ((crc & 0x80) != 0){
            record = true;
         }
         crc = crc << 1;

         if (record == true){
            crc ^= 0x07;
         }
      }
   }

   return crc;
}