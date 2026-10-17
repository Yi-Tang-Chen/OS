#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "frame_parser.h"
#include "crc8.h"

bool frame_parser_check(const uint8_t *data, size_t frame_size){
   if (data == NULL){
      return false;
   }
   if (frame_size < 7 || (data[0] != 0xA5 || data[1] != 0x5A)){
      return false;
   }

   if (data[2] != 0x01){
      return false;
   }

   if (data[3] > 32){
      return false;
   }

   if (frame_size != ((size_t)data[3] + 7)){
      return false;
   }
   
   size_t crc_position = 5 + (size_t)data[3] + 1;

   if (crc8(&data[2], (size_t)data[3] + 4) != data[crc_position]){
      return false;
   }

   return true;
}

void frame_parser_process(const uint8_t *data, size_t frame_size, void (*callback)(void)){
   if (callback == NULL){
      return;
   }
   if (frame_parser_check(data, frame_size)){
      callback();
   }
}

bool frame_parser_init(frame_parser_t *frame){
   if (frame == NULL){
      return false;
   }

   frame->used = 0;
   return true;
}

bool frame_parser_reset(frame_parser_t *frame){
   if (frame == NULL){
      return false;
   }
   for (size_t i = 0; i < 39; i++){
      frame->buffer[i] = 0;
   }
   frame->used = 0;
   return true;
}

bool frame_parser_feed(const uint8_t *data, 
                        size_t data_size, 
                        frame_parser_t *frame,
                        void (*callback)(void)){
                           
   if (data == NULL || frame == NULL){
      return false;
   }

   for (size_t i = 0; i < data_size; i++){
      if (frame->used >= sizeof(frame->buffer)){
         frame_parser_reset(frame);
         return false;
      }
      frame->buffer[frame->used] = data[i];
      frame->used++;

      if ((frame->used == 1 && frame->buffer[0] != 0xA5) ||
         (frame->used == 2 && frame->buffer[1] != 0x5A) ||
         (frame->used == 3 && frame->buffer[2] != 1)    ||
         (frame->used == 4 && frame->buffer[3] > 32)){
         frame_parser_reset(frame);
         return false;
      }

      if (frame->used < 4){
         continue;
      }

      size_t total_size = (size_t)frame->buffer[3] + 7;

      if (total_size > frame->used){
         continue;
      }

      if (!frame_parser_check(frame->buffer, total_size)){
         frame_parser_reset(frame);
         return false;
      }

      frame_parser_process(frame->buffer, total_size, callback);

      frame_parser_reset(frame);
   }

   return true;
}