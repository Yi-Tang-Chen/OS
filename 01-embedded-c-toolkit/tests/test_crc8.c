#include <assert.h>
#include <stdio.h>
#include <crc8.h>

static void test_empty_data(void){
   assert(crc8(NULL, 0) == 0);
}

static void test_one_byte(void){
   const uint8_t data[] = {0x01};

   assert(crc8(data, sizeof(data)) == 0x07);
}

static void test_known_vector(void)
{
   const uint8_t data[] = "123456789";
   
   assert(crc8(data, sizeof(data) - 1) == 0xF4);
}

static void test_normal(void){
   const uint8_t data1[] = {0x01, 0x02, 0x03};
   const uint8_t data2[] = {0x01, 0x00, 0x02};
   const uint8_t data3[] = {0x80};

   assert(crc8(data1, sizeof(data1)) == 0x48);
   assert(crc8(data2, sizeof(data2)) == 0x65);
   assert(crc8(data3, sizeof(data3)) == 0x89);
}

int main(void){
   test_empty_data();
   test_one_byte();
   test_known_vector();
   test_normal();

   puts("crc8 test passed");
   return 0;
}