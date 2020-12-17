#include <limits.h>
#include <plic.h>
#include <sha256.h>
#include <string.h>
#include <sysctl.h>
#include <uart.h>
#include "lz4.h"
#include "sdcard.h"

#define FLASH_CHUNK_SIZE (w25qxx_FLASH_SECTOR_SIZE)
#define FLASH_PACKAGE_SIZE (FLASH_CHUNK_SIZE + 4)

struct BufferLoop {};
struct Packet {
  uint32_t address;
  uint8_t buff[FLASH_CHUNK_SIZE];
};

static volatile uint8_t* receiving_buff = NULL;
static volatile struct Packet* writing_buff = NULL;
static volatile uint8_t is_receive_ok = 0;

static void receive_all(uint8_t* buff) {
  int got = 0;
  while (got < FLASH_CHUNK_SIZE + 4) {
    got +=
        uart_receive_data(STDIO_UART_NUM, buff + got, FLASH_PACKAGE_SIZE - got);
  }
}
static void core0_step2_read_serial() {
  uint8_t serial_buffer_a[FLASH_PACKAGE_SIZE] __attribute__((aligned(64)));
  uint8_t serial_buffer_b[FLASH_PACKAGE_SIZE] __attribute__((aligned(64)));
  receiving_buff = serial_buffer_a;

  uart_set_receive_trigger(STDIO_UART_NUM, UART_RECEIVE_FIFO_1);

  while (1) {
    while (writing_buff == (void*)receiving_buff)
      ;

    receive_all((uint8_t*)receiving_buff);

    is_receive_ok = 1;
    while (is_receive_ok)
      ;
    if (receiving_buff == serial_buffer_a) {
      receiving_buff = serial_buffer_b;
    } else {
      receiving_buff = serial_buffer_a;
    }
  }
}

static void sha_hex(char* in, char* out) {
  for (int i = 0; i < SHA256_HASH_LEN; i++) {
    sprintf(out + i * 2, "%02x", in[i]);
  }
}

// static void print4096buff(uint8_t* buff_ptr) {
//   printf("\ec===========================================\n");
//   for (int i = 0; i < 128; i++) {
//     for (int j = 0; j < 2; j++) {
//       for (int k = 0; k < 16; k++) {
//         printf("%02x", *buff_ptr);
//         buff_ptr++;
//       }
//       printf(" ");
//     }
//     printf("\n");
//   }
//   printf("===========================================\n");
// }

#define JSON_TYPE_2 ("{\"type\":2,\"hash\":\"%s\",\"address\":%d}\n")

#define LZ4_BLOCK_SIZE 1024
#define SD_SECTOR_SIZE 512
#define SECTOR_PER_LZ4_BLOCK (LZ4_BLOCK_SIZE / SD_SECTOR_SIZE)

static void core1_step2_write_flash() {
  uint8_t compare_buffer[FLASH_PACKAGE_SIZE] __attribute__((aligned(64)));
  uint8_t remain_buffer[FLASH_PACKAGE_SIZE] __attribute__((aligned(64)));
  uint32_t remain_size = 0;
  uint32_t last_package_size = 0;
  uint8_t hash_buffer[SHA256_HASH_LEN] __attribute__((aligned(64)));
  uint8_t hash_string_buff[SHA256_HASH_LEN * 2 + 1];
  int actualSize;

  uint8_t decBuf[LZ4_BLOCK_SIZE];
  LZ4_streamDecode_t lz4StreamDecode_body;
  LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;
  LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);


  sha256_hard_calculate("123", 3, hash_buffer);
  sha_hex(hash_buffer, hash_string_buff);

  char* outputBuffer = repeat_malloc(JSON_TYPE_2, hash_string_buff, UINT_MAX);
  int decBytes;

  // w25qxx_init(3, 0, 60000000);
  sd_init();

  uint32_t write_cnt = 0;

  while (1) {
    while (!is_receive_ok)
      ;
    writing_buff = (void*)receiving_buff;
    is_receive_ok = 0;

    uint32_t cursor = 0;

    while (cursor < FLASH_CHUNK_SIZE - 4) {

      if (remain_size) {
        if (remain_size < 4) {
          memcpy(remain_buffer + remain_size, writing_buff->buff, 4 - remain_size);
          remain_size = 4;
          cursor += 4 - remain_size;
        }
        last_package_size = *(uint32_t *)(remain_buffer);
        uint32_t new_package_size = last_package_size - remain_size;
        memcpy(remain_buffer + remain_size, writing_buff->buff + cursor, new_package_size);
        cursor += new_package_size;
        decBytes = LZ4_decompress_safe_continue(
          lz4StreamDecode, remain_buffer + 4, decBuf, last_package_size, LZ4_BLOCK_SIZE);
        remain_size = 0;
      } else {
        last_package_size = *(uint32_t *)(writing_buff->buff + cursor);
        if (cursor + last_package_size + 4> FLASH_CHUNK_SIZE) {
          remain_size = FLASH_CHUNK_SIZE - cursor;
          memcpy(remain_buffer, writing_buff->buff + cursor , remain_size);
          cursor = FLASH_CHUNK_SIZE;
          break;
        }
        cursor += 4;
        decBytes = LZ4_decompress_safe_continue(
          lz4StreamDecode, writing_buff->buff + cursor, decBuf, last_package_size, LZ4_BLOCK_SIZE);
        cursor += last_package_size;
      }

      sd_write_sector(decBytes, write_cnt, SECTOR_PER_LZ4_BLOCK);
      write_cnt += SECTOR_PER_LZ4_BLOCK;

    }

    if (cursor != FLASH_CHUNK_SIZE) {
      if (FLASH_CHUNK_SIZE - cursor < 4) {
        remain_size = FLASH_CHUNK_SIZE - cursor;
        *(uint32_t*)remain_buffer = *(uint32_t *)(writing_buff->buff + cursor);
      }
    }


    // // print4096buff((void*)writing_buff->buff);
    // w25qxx_write_data(writing_buff->address, (uint8_t*)writing_buff->buff,
    //                   FLASH_CHUNK_SIZE);

    // // msleep(400);
    // w25qxx_read_data(writing_buff->address, compare_buffer, FLASH_CHUNK_SIZE);
    // // print4096buff(compare_buffer);

    sha256_hard_calculate(writing_buff->buff, FLASH_CHUNK_SIZE, hash_buffer);
    sha_hex(hash_buffer, hash_string_buff);

    repeat_printf(outputBuffer, JSON_TYPE_2, hash_string_buff,
                  writing_buff->address);
  }
}
