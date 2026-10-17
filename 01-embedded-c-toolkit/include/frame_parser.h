#ifndef FRAME_PARSER
#define FRAME_PARSER

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct{
   uint8_t magic[2];
   uint8_t version;
   uint8_t length;
   uint16_t sequence;
   uint8_t payload[32];
   uint8_t crc;
}frame_t;

typedef struct{
   uint8_t buffer[39];
   size_t used;
}frame_parser_t;

bool frame_parser_check(const uint8_t *data, size_t frame_size);

bool frame_parser_init(frame_parser_t *frame);
bool frame_parser_reset(frame_parser_t *frame);
bool frame_parser_feed(const uint8_t *data, 
                        size_t data_size, 
                        frame_parser_t *frame,
                        void (*callback)(void));


#endif