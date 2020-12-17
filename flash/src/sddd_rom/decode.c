
// LZ4 streaming API example : double buffer
// Copyright : Takayuki Matsuoka


#if defined(_MSC_VER) && (_MSC_VER <= 1800)  /* Visual Studio <= 2013 */
#  define _CRT_SECURE_NO_WARNINGS
#  define snprintf sprintf_s
#endif
#include "lz4.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sdcard.h"
#include <syscalls.h>
#include "w25qxx.h"
#include "sleep.h"

uint32_t rom_addr = 1 << 20;  // 从1M开始是文件系统
const uint32_t fs_size = 1024 * 1024 * 2; // 文件系统大小为1M

enum {
    BLOCK_BYTES = 1024, 
//  BLOCK_BYTES = 1024 * 64,
};

#define BUF_SIZE 4096 
uint8_t rom_buf[BUF_SIZE];
uint32_t rom_cur = 0;
uint32_t rom_end = 0;

size_t read_rom(uint8_t *data, size_t length) {
    if (rom_cur + length > rom_end) {
        uint32_t len = rom_end - rom_cur;
        memcpy(data, rom_buf + rom_cur, len);
        w25qxx_read_data(rom_addr, rom_buf, BUF_SIZE);
        rom_addr += BUF_SIZE;
        rom_cur = len;
        rom_end = BUF_SIZE;
        return len + read_rom(data + len, length - len);
    } else {
        memcpy(data, rom_buf + rom_cur, length);
        rom_cur += length;
        return length;
    }
}

uint8_t cmp_buf[512];
size_t write_bin(const void* array, size_t arrayBytes) {
    static size_t sector_cnt = 0;
    const int sector_size = 512;
    int cnt = arrayBytes / sector_size;
    for (int i = 0; i < cnt; i++) {
        sd_write_sector(array + i * 512, sector_cnt, 1);
        sd_read_sector(cmp_buf, sector_cnt, 1);
        for (int j = 0; j < 512; j++) {
            uint8_t right = *(uint8_t*)(array + i * 512 + j);
            if (cmp_buf[j] != right) {
                printf("write sd err sd=%d right=%d sector=%ld offset=%d\n", cmp_buf[j], right, sector_cnt, j);
                exit(-1);
            }
        }
        sector_cnt += 1;
        // msleep(10);
    }
    // sector_cnt += cnt;
    return arrayBytes;
}




size_t read_bin(void* array, size_t arrayBytes) {
    // return read_rom(array, arrayBytes);
    w25qxx_read_data(rom_addr, array, arrayBytes);
    rom_addr += arrayBytes;
    return arrayBytes; 
 }

inline size_t read_int(int* i) {
    // return read_rom((uint8_t*)i, sizeof(int));
    return read_bin((void*)i, 4);
}



size_t decompress()
{
    LZ4_streamDecode_t lz4StreamDecode_body;
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

    char decBuf[BLOCK_BYTES];

    LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);
    size_t cnt = 0;
    printf("start receive data\n");
    for(;;) {
        char cmpBuf[LZ4_COMPRESSBOUND(BLOCK_BYTES)];
        int  cmpBytes = 0;

        {
            read_int(&cmpBytes);
            // printf("block size %d\n", cmpBytes);
            // const size_t readCount0 = scanf("%d", &cmpBytes);
            printf("cmpBytes %d\n", cmpBytes);
            // if(readCount0 != 1 || cmpBytes <= 0) {
            //     printf("res:1\n");
            //     continue;
            //     // break;
            // }

            read_bin(cmpBuf, (size_t) cmpBytes);
            // printf("cmpBytes %d\n", cmpBytes);
            // printf("readCount0:%ld, readCount1:%ld, cmpBytes:%d\n", readCount0, readCount1, cmpBytes);
        }

        {
            char* const decPtr = decBuf;
            const int decBytes = LZ4_decompress_safe_continue(
                lz4StreamDecode, cmpBuf, decPtr, cmpBytes, BLOCK_BYTES);
            if (decBytes != BLOCK_BYTES) {
                printf("decBytes = %d\n", decBytes);
                exit(-1);
            }

            cnt += write_bin(decPtr, (size_t) decBytes);
            printf("%ld/%d\n", cnt, fs_size);
            if (cnt >= fs_size) {
                break;
            }
            // printf("receive %ld\n", cnt);
        }

    }
    return cnt;
}
