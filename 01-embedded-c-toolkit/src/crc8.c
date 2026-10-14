#include<stdint.h>
#include<stddef.h>
#include<stdbool.h>

uint8_t crc8(uint8_t *data, size_t length){
   if (length != 0 && data == NULL){
      return NULL;
   }
   uint8_t crc = 0x00;

   crc = crc ^ data[0];
}