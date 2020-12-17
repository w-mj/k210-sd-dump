
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

enum {
    BLOCK_BYTES = 1024, 
//  BLOCK_BYTES = 1024 * 64,
};


size_t write_int(FILE* fp, int i) {
    // return fwrite(&i, sizeof(i), 1, fp);
    return 0;
}

size_t write_bin(const void* array, size_t arrayBytes) {
    static size_t sector_cnt = 0;
    const int sector_size = 512;
    int cnt = arrayBytes / sector_size;
    sd_write_sector(array, sector_cnt, cnt);
    sector_cnt += cnt;
    return sector_cnt;
}


size_t serial_num = 0;
size_t read_int(FILE* fp, int* i) {
    return fread(i, sizeof(*i), 1, fp);
}

size_t read_bin(FILE* fp, void* array, size_t arrayBytes) {
    return fread(array, 1, arrayBytes, fp);
}



size_t decompress()
{
    FILE *inpFp = stdin;
    LZ4_streamDecode_t lz4StreamDecode_body;
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

    char decBuf[2][BLOCK_BYTES];
    int  decBufIndex = 0;

    LZ4_setStreamDecode(lz4StreamDecode, NULL, 0);
    size_t cnt = 0;
    printf("start receive data\n");
    for(;;) {
        char cmpBuf[LZ4_COMPRESSBOUND(BLOCK_BYTES)];
        int  cmpBytes = 0;

        {
            const size_t readCount0 = read_int(inpFp, &cmpBytes);
            serial_num += readCount0;
            // const size_t readCount0 = scanf("%d", &cmpBytes);
            printf("readCount0 %ld, cmpBytes %d\n", readCount0, cmpBytes);
            if(readCount0 != 1 || cmpBytes <= 0) {
                sys_stdin_flush();
                printf("res:1\n");
                continue;
                // break;
            }

            const size_t readCount1 = read_bin(inpFp, cmpBuf, (size_t) cmpBytes);
            serial_num += readCount1;
            printf("read count 1 %ld, cmpBytes %d\n", readCount1, cmpBytes);
            if(readCount1 != (size_t) cmpBytes) {
                sys_stdin_flush();
                printf("res:2\n");
                continue;
            }
            printf("readCount0:%ld, readCount1:%ld, cmpBytes:%d\n", readCount0, readCount1, cmpBytes);
        }

        {
            char* const decPtr = decBuf[decBufIndex];
            const int decBytes = LZ4_decompress_safe_continue(
                lz4StreamDecode, cmpBuf, decPtr, cmpBytes, BLOCK_BYTES);
            printf("decBytes %d\n", decBytes);
            if(decBytes <= 0) {
                sys_stdin_flush();
                printf("res:3\n");
                continue;
            }
            cnt += write_bin(decPtr, (size_t) decBytes);
            printf("res:0\n");
            // printf("receive %ld\n", cnt);
        }

        decBufIndex = (decBufIndex + 1) % 2;
    }
    return cnt;
}
